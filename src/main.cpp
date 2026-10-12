#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "RK35llm.h"

int main(int argc, char** argv)
{
    std::string input_str;
    std::string output_str;
    RK35llm RKLLM;

    RKLLM.SetInfo(true);
    RKLLM.SetSilence(false);

    if     (argc< 4) {std::cerr << "Usage: " << argv[0] << " image vlm_model llm_model [option]NewTokens [option]ContextLength\n"; return -1;}
    else if(argc==4) RKLLM.LoadModel(argv[2],argv[3]);
    else if(argc==5) RKLLM.LoadModel(argv[2],argv[3],std::atoi(argv[4]));
    else if(argc> 5) RKLLM.LoadModel(argv[2],argv[3],std::atoi(argv[4]),std::atoi(argv[5]));

    // 用 stb_image 读图（不依赖 OpenCV JPEG 解码器）
    int width, height, channels;
    unsigned char* img_data = stbi_load(argv[1], &width, &height, &channels, 3);
    if (!img_data) {
        std::cerr << "Failed to load image: " << argv[1] << std::endl;
        return -1;
    }
    cv::Mat Pic(height, width, CV_8UC3, img_data);
    RKLLM.LoadImage(Pic);
    stbi_image_free(img_data);

    while(true) {
        printf("\n");
        printf("User: ");

        std::getline(std::cin, input_str);
        if (input_str == "exit") break;

        output_str = RKLLM.Ask(input_str);
    }

    return 0;
}
