#include "MediaHandle.h"
#include <iostream>
#include <future>

int main()
{
    std::shared_ptr<CMediaHandle> spMediaHandle = std::make_shared<CMediaHandle>("./test.mp4");

    LONG lRet = FF_SUCCESS;
    do 
    {
   
        lRet = spMediaHandle->InitMediaInfo();
        if (FF_SUCCESS!=lRet)
        {
            break;
        }
        lRet = spMediaHandle->InitDecoder();
        if (FF_SUCCESS != lRet)
        {
            break;
        }
        lRet= spMediaHandle->InitEncoder();
        if (FF_SUCCESS != lRet)
        {
            break;
        }
        lRet = spMediaHandle->InitOutputFormatInfo();
        if (FF_SUCCESS != lRet)
        {
            break;
        }
        std::weak_ptr<CMediaHandle> spWeakHandle = spMediaHandle;
        auto fut = std::async(std::launch::async, [spWeakHandle]() {
            std::shared_ptr<CMediaHandle> spHandle = spWeakHandle.lock();
            if (spHandle)
            {
                spHandle->Run();
            }
            });
        fut.wait();
    } while (0);

    return 1;
}