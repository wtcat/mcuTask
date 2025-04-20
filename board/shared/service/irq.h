/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_IRQ_H_
#define SERVICE_IRQ_H_

#ifdef __cplusplus
extern "C"{
#endif

int  init_irq(void);
int  enable_irq(int irq);
int  disable_irq(int irq);
void dispatch_irq(void);
int  request_irq(int irq, void (*handler)(void *), void *arg);
int  remove_irq(int irq, void (*handler)(void *), void *arg);

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_IRQ_H_ */
