#include "thread_utils.h"
#include "lf_queue.h"

struct MyStruct{
    int d_[3];
};

using namespace Common;
auto ConsumeFunction(LFQueue<MyStruct>* lfq){
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    while(lfq->Size()){
        const auto d = lfq->GetNextToRead();
        lfq->UpdateReadIndex();
        std::cout << "ConsumeFunction read elem: " << d->d_[0] << ", " << d->d_[1] << ", " << d->d_[2] << " lfq-size: " << lfq->Size() << std::endl;
        std::this_thread::sleep_for(1s);
    }
    std::cout << "ConsumeFunction exiting." << std::endl;
}

int main(int, char**){
    LFQueue<MyStruct>lfq(20);
    auto ct = CreateAndStartThread(-1, "", ConsumeFunction, &lfq);
    for(auto i = 0; i < 50; ++i){
        const MyStruct d{i, i * 10, i * 100};
        *(lfq.GetNextToWriteTo()) = d;
        lfq.UpdateWriteIndex();
        std::cout << "main constructed elem: " << d.d_[0] << ", " << d.d_[1] << ", " << d.d_[2] << " lfq-size: " << lfq.Size() << std::endl;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    ct->join();
    std::cout << "main exiting." << std::endl;
    return 0;
}