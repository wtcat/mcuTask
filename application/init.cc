/*
 * Copyright 2025 wtcat
 */

#include "tx_api.h"

#include "base/containers/linked_list.h"


class LinkTest: public base::LinkNode<LinkTest> {

};

__externC int main(void) {
    base::LinkedList<LinkTest> list;
    LinkTest anode;
    LinkTest bnode;

    list.Append(&anode);

    while (1)
        tx_thread_sleep(TX_MSEC(1000));

    return 0;
}