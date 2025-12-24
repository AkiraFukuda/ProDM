#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <cmath>
#include <bitset>

#include "utils.hpp"
#include "MDR/Refactor/Refactor.hpp"
#include "MDR/Reconstructor/Reconstructor.hpp"

using namespace std;
bool negabinary = true;

template <class T, class Refactor>
uint32_t evaluate_refactor_to_buffer(const vector<T> &data,
                                     const vector<uint32_t> &dims,
                                     int target_level,
                                     int num_bitplanes,
                                     Refactor &refactor,
                                     uint8_t *buffer)
{
    struct timespec start, end;
    int err = 0;

    cout << "Start refactoring to buffer" << endl;
    err = clock_gettime(CLOCK_REALTIME, &start);
    (void)err;

    uint32_t buffer_size =
        refactor.refactor_to_buffer(data.data(), dims,
                                    static_cast<uint8_t>(target_level),
                                    static_cast<uint8_t>(num_bitplanes),
                                    buffer);

    err = clock_gettime(CLOCK_REALTIME, &end);
    double t =
        (double)(end.tv_sec - start.tv_sec) +
        (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

    cout << "Refactor time: " << t << "s" << endl;
    cout << "Buffer size = " << buffer_size << " bytes" << endl;

    return buffer_size;
}

template <class T, class Reconstructor>
void evaluate_reconstruct_from_buffer(const vector<T> &data,
                                      const vector<double> &tolerance,
                                      Reconstructor &reconstructor,
                                      const uint8_t *buffer)
{
    struct timespec start, end;
    int err = 0;

    for (int i = 0; i < (int)tolerance.size(); i++)
    {
        cout << "Start reconstruction from buffer, tolerance = "
             << tolerance[i] << endl;

        err = clock_gettime(CLOCK_REALTIME, &start);
        (void)err;

        auto reconstructed_data =
            reconstructor.reconstruct_from_buffer(tolerance[i], buffer);

        err = clock_gettime(CLOCK_REALTIME, &end);
        double t =
            (double)(end.tv_sec - start.tv_sec) +
            (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
        cout << "Reconstruct time: " << t << "s" << endl;

        if (reconstructed_data == NULL)
        {
            cout << "Reconstruction failed, skip statistics." << endl;
            continue;
        }

        auto dims = reconstructor.get_dimensions();
        cout << "Dimensions: ";
        for (auto d : dims)
            cout << d << " ";
        cout << endl;

        MGARD::print_statistics(data.data(), reconstructed_data,
                                data.size());
    }
}

template <class T, class Decomposer, class Interleaver, class Encoder,
          class Compressor, class ErrorCollector, class ErrorEstimator,
          class SizeInterpreter, class Writer, class Retriever>
void test_buffer(string filename,
                 const vector<uint32_t> &dims,
                 int target_level,
                 int num_bitplanes,
                 const vector<double> &tolerance,
                 Decomposer decomposer,
                 Interleaver interleaver,
                 Encoder encoder,
                 Compressor compressor,
                 ErrorCollector collector,
                 ErrorEstimator estimator,
                 SizeInterpreter interpreter,
                 Writer writer,
                 Retriever retriever)
{
    size_t num_elements = 0;
    auto data = MGARD::readfile<T>(filename.c_str(), num_elements);
    cout << "read file done: #element = " << num_elements << endl;
    fflush(stdout);

    auto refactor =
        MDR::OrderedRefactor<T, Decomposer, Interleaver, Encoder,
                             Compressor, ErrorCollector, ErrorEstimator, Writer>(
            decomposer, interleaver, encoder,
            compressor, collector, estimator, writer);
    refactor.negabinary = negabinary;

    uint32_t estimated_data_size = num_elements * sizeof(T) + 1024;
    cout << "Allocated size = " << estimated_data_size << endl;
    uint8_t * buffer = (uint8_t*)malloc(sizeof(uint8_t) * estimated_data_size);
    uint32_t buffer_size = evaluate_refactor_to_buffer<T>(
        data, dims, target_level, num_bitplanes, refactor, buffer);

    auto reconstructor =
        MDR::OrderedReconstructor<T, Decomposer, Interleaver, Encoder,
                                  Compressor, SizeInterpreter, ErrorEstimator,
                                  Retriever>(
            decomposer, interleaver, encoder,
            compressor, interpreter, retriever);

    evaluate_reconstruct_from_buffer<T>(data, tolerance,
                                        reconstructor, buffer);

    free(buffer);
}

int main(int argc, char **argv)
{
    int argv_id = 1;
    if (argc < 6)
    {
        cout << "Usage: " << argv[0]
             << " filename target_level num_bitplanes num_dims dims... "
             << "num_tolerance tol1 tol2 ..." << endl;
        return 0;
    }

    string filename = string(argv[argv_id++]);
    int target_level = atoi(argv[argv_id++]);
    int num_bitplanes = atoi(argv[argv_id++]);

    // 与原测试保持一致：bitplane 数为偶数，且限制最大 bitplane 数
    if (num_bitplanes % 2 == 1)
    {
        num_bitplanes += 1;
        std::cout << "Change to " << num_bitplanes
                  << " bitplanes for simplicity of negabinary encoding"
                  << std::endl;
    }

    int num_dims = atoi(argv[argv_id++]);
    vector<uint32_t> dims(num_dims, 0);
    for (int i = 0; i < num_dims; i++)
    {
        dims[i] = atoi(argv[argv_id++]);
    }

    int num_tolerance = atoi(argv[argv_id++]);
    vector<double> tolerance(num_tolerance, 0.0);
    for (int i = 0; i < num_tolerance; i++)
    {
        tolerance[i] = atof(argv[argv_id++]);
    }

    // 下面的类型和配置基本直接照搬原来的测试代码
    string metadata_file = "refactored_data/metadata.bin";
    string data_file = "refactored_data/data.bin";

    using T = float;
    using T_stream = uint32_t;

    if (num_bitplanes > 32)
    {
        num_bitplanes = 32;
        std::cout << "Only less than 32 bitplanes are supported for "
                     "single-precision floating point"
                  << std::endl;
    }

    // using T = double;
    // using T_stream = uint64_t;
    // if(num_bitplanes > 64){
    //     num_bitplanes = 64;
    //     std::cout << "Only less than 64 bitplanes are supported for "
    //                  "double-precision floating point" << std::endl;
    // }

    auto decomposer = MDR::MGARDHierarchicalDecomposer<T>();
    auto interleaver = MDR::DirectInterleaver<T>();
    // auto interleaver = MDR::SFCInterleaver<T>();
    // auto interleaver = MDR::BlockedInterleaver<T>();

    auto encoder = MDR::NegaBinaryBPEncoder<T, T_stream>();
    negabinary = true;
    // auto encoder = MDR::PerBitBPEncoder<T, T_stream>();
    // negabinary = false;

    // auto compressor = MDR::DefaultLevelCompressor();
    auto compressor = MDR::AdaptiveLevelCompressor(64);
    // auto compressor = MDR::NullLevelCompressor();

    auto collector = MDR::SquaredErrorCollector<T>();
    auto estimator = MDR::MaxErrorEstimatorHB<T>();

    // Writer/Retriever 仍然使用原来的文件版本，但在本测试中不会真正访问文件
    auto writer = MDR::OrderedFileWriter(metadata_file, data_file);
    auto retriever = MDR::OrderedFileRetriever(metadata_file, data_file);

    auto interpreter =
        MDR::SignExcludeGreedyBasedSizeInterpreter<
            MDR::MaxErrorEstimatorHB<T>>(estimator);

    test_buffer<T>(filename, dims, target_level, num_bitplanes,
                   tolerance,
                   decomposer, interleaver, encoder, compressor,
                   collector, estimator, interpreter, writer, retriever);
    return 0;
}
