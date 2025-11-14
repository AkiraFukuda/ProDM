#ifndef _MDR_ORDERED_FILE_RETRIEVER_HPP
#define _MDR_ORDERED_FILE_RETRIEVER_HPP

#include "RetrieverInterface.hpp"
#include <cstdio>

namespace MDR {
    // Data retriever for files
    class OrderedFileRetriever : public concepts::RetrieverInterface {
    public:
        OrderedFileRetriever(const std::string& metadata_file, const std::string& data_file) : metadata_file(metadata_file), data_file(data_file) {}

        uint8_t * retrieve_level_components(uint32_t retrieve_size) {
            release();
            FILE * file = fopen(data_file.c_str(), "rb");
            if(fseek(file, offset, SEEK_SET)){
                std::cerr << "Errors in fseek while retrieving from file" << std::endl;
            }
            uint8_t * buffer = (uint8_t *) malloc(retrieve_size);
            fread(buffer, sizeof(uint8_t), retrieve_size, file);
            components = buffer;
            fclose(file);
            offset += retrieve_size;
            total_retrieved_size += offset;
            return components;
        }
        
        uint8_t * load_metadata() const {
            FILE * file = fopen(metadata_file.c_str(), "rb");
            fseek(file, 0, SEEK_END);
            uint32_t num_bytes = ftell(file);
            rewind(file);
            uint8_t * metadata = (uint8_t *) malloc(num_bytes);
            fread(metadata, 1, num_bytes, file);
            fclose(file);
            return metadata;
        }

        void release(){
            free(components);
        }

        size_t get_retrieved_size(){
            return total_retrieved_size;
        }
        
        uint32_t get_offset(){
            return offset;
        }

        ~OrderedFileRetriever(){}

        void print() const {
            std::cout << "Ordered file retriever." << std::endl;
        }
    private:
        std::string metadata_file;
        std::string data_file;
        uint32_t offset;
        uint8_t* components;
        size_t total_retrieved_size = 0;
    };
}
#endif
