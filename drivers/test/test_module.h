#ifndef MY_DEVICE_DRIVER_H_
#define MY_DEVICE_DRIVER_H_
/*ここからhttps://qiita.com/iwatake2222/items/ade0a73d4c05fc7961d3 の引用*/
#include <linux/ioctl.h>

/*** ioctl用パラメータ(第3引数)の定義 ***/
struct mydevice_values {
	int val1;
	int val2;
};

/*** ioctl用コマンド(request, 第2引数)の定義 ***/
/* このデバイスドライバで使用するIOCTL用コマンドのタイプ。なんでもいいが、'M'にしてみる */
#define MYDEVICE_IOC_TYPE 'M'

/* デバドラに値を設定するコマンド。パラメータはmydevice_values型 */
#define MYDEVICE_SET_VALUES _IOW(MYDEVICE_IOC_TYPE, 1, struct mydevice_values)

/* デバドラから値を取得するコマンド。パラメータはmydevice_values型 */
#define MYDEVICE_GET_VALUES _IOR(MYDEVICE_IOC_TYPE, 2, struct mydevice_values)

#define TESTDEVICE_STATE_WRITE _IOW(MYDEVICE_IOC_TYPE, 3, enum testdevice_state)

#define TESTDEVICE_STATE_READ _IOR(MYDEVICE_IOC_TYPE, 4, enum testdevice_state)

#define TESTDEVICE_FIFO_CLEAN _IOW(MYDEVICE_IOC_TYPE, 5, enum testdevice_state)
/*ここまでhttps://qiita.com/iwatake2222/items/ade0a73d4c05fc7961d3 の引用*/
enum testdevice_state {
	TESTDEVICE_STATE_DISABLE,
	TESTDEVICE_STATE_ENABLE,
};
#endif /* MY_DEVICE_DRIVER_H_ */
