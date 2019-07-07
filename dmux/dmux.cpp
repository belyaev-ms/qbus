#include "dmux/message.h"
#include "dmux/queue.h"

using namespace dmux::queue;

int main(int argc, char** argv)
{
    simple_queue queue(512, 1024);
    pqueue_type pqueue = make<simple_queue>(512, 1024);
    pqueue->add(NULL, 100);
    return 0;
}

