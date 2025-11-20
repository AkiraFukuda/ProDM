#include <adios2.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

static bool approx_equal(const std::vector<float>& a,
                         const std::vector<float>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::memcmp(&a[i], &b[i], sizeof(float)) != 0)
            return false;
    return true;
}

static std::vector<float> load_bin_3d(const std::string& binfile,
                                     size_t d0, size_t d1, size_t d2)
{
    const size_t N = d0 * d1 * d2;
    std::vector<float> data(N);

    std::ifstream ifs(binfile, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("cannot open " + binfile);

    ifs.read(reinterpret_cast<char*>(data.data()),
             static_cast<std::streamsize>(N * sizeof(float)));

    if (ifs.gcount() != static_cast<std::streamsize>(N * sizeof(float)))
        throw std::runtime_error("file size mismatch when reading " + binfile);

    return data;
}

static void write_bp(const std::string& fname,
                     const std::vector<float>& data,
                     size_t d0, size_t d1, size_t d2)
{
    adios2::ADIOS ad;
    adios2::IO io = ad.DeclareIO("ioWriter");
    io.SetEngine("BP5");

    const adios2::Dims shape{d0, d1, d2};
    const adios2::Dims start{0, 0, 0};
    const adios2::Dims count{d0, d1, d2};

    auto var = io.DefineVariable<float>("x", shape, start, count);

    // loading operator
    adios2::Params p;
    p["PluginName"] = "MdrOperator";
    p["PluginLibrary"] = "MdrOperator";
    var.AddOperation("plugin", p);

    auto writer = io.Open(fname, adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(var, data.data());
    writer.EndStep();
    writer.Close();
}

static std::vector<float> read_bp(const std::string& fname,
                                 size_t d0, size_t d1, size_t d2)
{
    std::vector<float> out(d0 * d1 * d2, 0.0f);

    adios2::ADIOS ad;
    adios2::IO io = ad.DeclareIO("ioReader");
    io.SetEngine("BP5");

    auto reader = io.Open(fname, adios2::Mode::Read);
    reader.BeginStep();

    auto var = io.InquireVariable<float>("x");
    if (!var) throw std::runtime_error("variable x not found");

    var.SetSelection({{0, 0, 0}, {d0, d1, d2}});
    reader.Get(var, out.data(), adios2::Mode::Sync);

    reader.EndStep();
    reader.Close();

    return out;
}

int main(int argc, char** argv)
{
    try {
        const std::string fname   = "test.bp";
        const std::string binfile = "data.bin";

        constexpr size_t D0 = 115;
        constexpr size_t D1 = 4761;
        constexpr size_t D2 = 288;

        // ----------------- Load from data.bin -----------------
        std::vector<float> data = load_bin_3d(binfile, D0, D1, D2);

        // ----------------- Write -----------------
        write_bp(fname, data, D0, D1, D2);

        // ----------------- Read -----------------
        std::vector<float> out = read_bp(fname, D0, D1, D2);

        std::cout << (approx_equal(data, out) ? "verified\n"
                                              : "data mismatch\n");
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
