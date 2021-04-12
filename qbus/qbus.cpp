#include "qbus/message.h"
#include "qbus/queue.h"

using namespace qbus::queue;

int main(int argc, char** argv)
{
    simple_queue queue(512, 1024);
    pqueue_type pqueue = make<simple_queue>(512, 1024);
    pqueue->add(NULL, 100);
    return 0;
}

