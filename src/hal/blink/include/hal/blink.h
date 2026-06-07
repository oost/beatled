#ifndef HAL_BLINK_BLINK_H
#define HAL_BLINK_BLINK_H

#ifdef __cplusplus
extern "C" {
#endif

void blink_once(int speed);
void blink(int speed, int count);

#ifdef __cplusplus
}
#endif
#endif // HAL_BLINK_BLINK_H