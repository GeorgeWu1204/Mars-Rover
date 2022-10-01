#include<string>

class fpga {
    public:
        void distance_decode(std::string received_message, int &colour, int &distance);
        void pixel_decode(std::string received_message, int &colour, int &pixel);
        
};

