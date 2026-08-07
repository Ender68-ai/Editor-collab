#include <Geode/Geode.hpp>
#include <support/zip_support/ZipUtils.h>
int main() {
    unsigned char* out = nullptr;
    ssize_t outSize = 0;
    // test if it compiles
    cocos2d::ZipUtils::ccCompressMemory((unsigned char*)"test", 4, &out, &outSize);
    return 0;
}
