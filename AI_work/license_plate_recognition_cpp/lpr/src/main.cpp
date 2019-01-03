#include "../include/Pipeline.h"

#include "CvxText.h"
#include "opencv2/opencv.hpp"

using namespace cv;
//中文显示
static int ToWchar(char* &src, wchar_t* &dest, const char *locale = "zh_CN.utf8")
{
    if (src == NULL) {
        dest = NULL;
        return 0;
    }

    // 根据环境变量设置locale
    setlocale(LC_CTYPE, locale);

    // 得到转化为需要的宽字符大小
    int w_size = mbstowcs(NULL, src, 0) + 1;

    // w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能使locale
    // 没有设置正确)
    if (w_size == 0) {
        dest = NULL;
        return -1;
    }

    //wcout << "w_size" << w_size << endl;
    dest = new wchar_t[w_size];
    if (!dest) {
        return -1;
    }

    int ret = mbstowcs(dest, src, strlen(src)+1);
    if (ret <= 0) {
        return -1;
    }
    return 0;
}

int main(){
    pr::PipelinePR prc("model/cascade.xml",
                      "model/HorizonalFinemapping.prototxt","model/HorizonalFinemapping.caffemodel",
                      "model/Segmentation.prototxt","model/Segmentation.caffemodel",
                      "model/CharacterRecognization.prototxt","model/CharacterRecognization.caffemodel",
                       "model/SegmentationFree.prototxt","model/SegmentationFree.caffemodel"
                    );
  //定义模型文件

    //设置中文格式
    CvxText text("simhei.ttf"); //指定字体
    cv::Scalar size1{ 20, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
    text.setFont(nullptr, &size1, nullptr, 0);

    cv::Mat image = cv::imread("../test/1.jpg");
    std::vector<pr::PlateInfo> res = prc.RunPiplineAsImage(image,pr::SEGMENTATION_FREE_METHOD);
  //使用端到端模型模型进行识别 识别结果将会保存在res里面
 
    for(auto st:res) {
        if(st.confidence>0.75) {
            std::cout << "置信度为：" << st.confidence << std::endl;
          //输出识别结果 、识别置信度
            cv::Rect region = st.getPlateRect();
          //获取车牌位置
            cv::rectangle(image,cv::Point(region.x,region.y),cv::Point(region.x+region.width,region.y+region.height),cv::Scalar(255,255,0),2);
          //画出车牌位置
            std::string s1 = st.getPlateName();
            char *data;
            int len = s1.length();
            data = (char *)malloc((len + 1)*sizeof(char));
            s1.copy(data, len, 0);
            data[len] = '\0';
            char* str1 = data;
            wchar_t *w_str1;
            ToWchar(str1,w_str1);
            text.putText(image, w_str1, cv::Point(region.x,region.y-5), cv::Scalar(255, 0, 0));
          //显示结果
        }
    }

    cv::imshow("image",image);
    cv::waitKey(0);
    return 0 ;
}
