# 【电子森林项目】网络收音机/FM收音机

这个项目是报名[《硬禾“暑期一起练”第3个平台 - 基于ESP32-S2模块的网络收音机和音频信号处理》](https://mp.weixin.qq.com/s/ED1UDWq3EFQPCbI2P3rHdw)所做的。

<img src="/docs/images/0.png" style="zoom:80%;" />

**基本功能**：

- 可以连接WiFi收听HLS协议的网络电台节目
- 收听空中的FM电台88MHz~108MHz
- OLED0.96寸显示
- 四个独立按键控制两种模式，切换节目，静音。

**项目环境**：

- [ESP-IDF](https://github.com/espressif/esp-idf) v4.2：乐鑫ESP系列的基本开发环境，借鉴了`sntp`例程。
- [ESP-ADF](https://github.com/espressif/esp-adf) v2.3：乐鑫的音频开发框架，主要借鉴了`pipeline_living_stream`和`pipeline_play_mp3_with_dac_or_pwm`这两个例程。
- [ESP-IOT-SOLUTION](https://github.com/espressif/esp-iot-solution)：Iot设备驱动和解决方案。使用`SSD1306`库，里面的7.5版本的LVGL已经被我用8.1的替换，并且修改一处SSD1306驱动程序的错误（[ssd1306水平终止范围错误 #103](https://github.com/espressif/esp-iot-solution/issues/103)）以及使用`iot_button`库，因为这个驱动可以识别按钮单击、双击、长按等功能。
- [LVGL](https://github.com/lvgl/lvgl) v8.1：嵌入式GUI库，替换ESP-IOT-SOLUTION中的v7.5的LVGL。
- [ESP-IDF-LIB](https://github.com/UncleRus/esp-idf-lib) v0.8.1：基于IDF开发的一些常用芯片的库（项目中使用RDA5807M的库）。
- [MCU_Font_Release](https://gitee.com/WuBinCPP/MCU_Font_Release)：用于生成LVGL使用的自定义字体来显示中文。

**硬件**：

ESP32-S2-MINI-1：ESP32-S2-FH4的芯片，320K的DRAM,无PSRAM。

FM模块：RDA5807M 

> 这个项目仅仅是学习目的，IDF也只是第二次接触，其他库第一次接触，本人编程功底也很烂，项目中肯定有不合适的地方，请大家多多指教。

## 1 如何使用

> 项目中的IDF和ADF环境需要自己按照官方教程进行配置，其他的比如iot-solution以及LVGL以及内置到项目中。下面是用的VS Code的esp-idf插件作为开发环境。

### 1.1 克隆项目

```
git clone https://github.com/DaMiBear/ESP32S2_Network-Radio_FM-Radio.git
```

### **1.2 选择设备**

VS Code打开项目文件夹后，F1输入如下命令，选择根据你的设备选择，项目中使用的是ESP32S2。

<img src="/docs/images/1.png" style="zoom:80%;" />

### 1.3 配置

点击如下图标，进行设置

<img src="/docs/images/2.png" alt="2" style="zoom:80%;" />

###### Serial flasher config

Flash size改为4MB

###### Partition Table

更改为如下设置，默认的factory大小只有1M是不够的，要改为3M

<img src="/docs/images/14.png" alt="14" style="zoom:80%;" />

###### Audio HAL

选择ESP32-S2-Kaluga-1，这个选项是属于ADF里的，ADF里大多数程序都是按照官方提供的那些开发板来的，官方开发板中的S2是有2MB的PSRAM的，这里主要是为了产生一些宏定义防止编译出错，具体初始化的程序是我们自己写的。

<img src="/docs/images/3.png" style="zoom:80%;" />

###### play_living_stream Configuration

输入自己WiFi的名称和密码。如果有中文，就在sdkconfig这个文件中修改，**注意**在sdkconfig文件中修改只限于WiFi名称和密码，其他的值大部分会在编译的时候重新覆盖。

###### Compiler options

为了节省内存，这里选择.

> 因为ADF的play_living_stream程序非常占内存，再加上FreeRTOS和LVGL，如果是默认设置，320K的内存是肯定不够的，所以要更改一些配置来节省内存。

<img src="/docs/images/4.png" alt="4" style="zoom:80%;" />

###### ESP32S2-specific

选择240MHz

<img src="/docs/images/5.png"  style="zoom: 80%;" />

###### Wi-Fi

与IRAM有关的都去掉，要不然代码会直接放入内存中，虽然可以加快速度，但是内存是真的不够用。

<img src="/docs/images/6.png"  style="zoom: 80%;" />

###### LCD Drivers

勾选`SSD1306`

###### LVGL configuration

勾选`LVGL minimal configuration`

###### Color settings

选择1:1

###### Memory settings

设置为6，默认的大小为32，就是32K，这样就会产生一个大小为32K的全局变量，这个才是内存占用的罪魁祸首。一开始没发现这一点，导致加入LVGL程序后，内存根本不够用。这里设置为6可以不报错，试过5或者更小，但在运行过程中会出错。

<img src="/docs/images/7.png"  style="zoom: 80%;" />

###### Feature configuration - Others

勾选`Enable float in built-in (v)snprintf functions`来支持浮点数显示

###### Font usage - Enable built-in fonts

选用`Enable Montserrat 20`和`Enable UNSCII 8 (Perfect monospace font)`，因为项目中使用的LVGL内置字体就这两个，还有一个为了支持中文显示的自定义字体。

下面的Select theme default title font默认格式选择`UNSCII 8`。这里LVGL里面的Kconfig文件中，比如我们选择的是`UNSCII 8`那就会产生一个`CONFIG_LV_FONT_DEFAULT_UNSCII_8`的宏定义，之后根据`lv_conf_kconfig.h`文件的`#elif defined`会定义

```c
/* file:lv_conf_kconfig.h */
#elif defined CONFIG_LV_FONT_DEFAULT_UNSCII_8
#define CONFIG_LV_THEME_DEFAULT_FONT         &lv_font_unscii_8
```

随后会根据`lv_conf_internal.h`文件中下面部分代码，但是代码中使用的是`CONFIG_LV_FONT_DEFAULT`，而不是`CONFIG_LV_THEME_DEFAULT_FONT     `，因为`CONFIG_LV_FONT_DEFAULT`没有定义，那这样设置默认字体是没效果的。所以我把`CONFIG_LV_FONT_DEFAULT`改为了`CONFIG_LV_THEME_DEFAULT_FONT`，这样就可以改变默认字体了，当然这肯定不是最好的办法。

```c
/* file:lv_conf_internal.h */
/*Always set a default font*/
#ifndef LV_FONT_DEFAULT
#  ifdef CONFIG_LV_FONT_DEFAULT		// 更改为了CONFIG_LV_THEME_DEFAULT_FONT
#    define LV_FONT_DEFAULT CONFIG_LV_FONT_DEFAULT	// 更改为了CONFIG_LV_THEME_DEFAULT_FONT
#  else
#    define  LV_FONT_DEFAULT &lv_font_montserrat_8
#  endif
#endif
```

> 后面的LVGL选项自己根据情况勾选就行。

### 1.4 编译下载

点击左下角图小，分辨是编译、下载、监视，第四个是三个功能合为一个按键。

<img src="/docs/images/8.png"  style="zoom: 80%;" />

## 2 运行现象

从左到右四个按键功能依次是：网络电台/FM电台模式切换、切换至下一个节目/频段、切换至上一个节目/频段、禁用功放（静音）。

默认开机运行网络电台模式，该模式下，显示屏显示一些电台名称、时间等信息。FM电台模式下显示频段、RSSI、时间信息。

**日志输出**

WiFi的输出省略

```
I (439) cpu_start: Starting scheduler on PRO CPU.
I (441) gpio: GPIO[41]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (441) gpio: GPIO[42]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (461) spi_bus: SPI2 bus created
I (461) spi_bus: SPI2 bus device added, CS=38 Mode=0 Speed=20000000
I (661) lvgl_gui: GUI Run at esp32s2 Pinned to Core0
I (661) lvgl adapter: Alloc memory total size: 1024 Byte
I (661) lvgl_gui: Start to run LVGL
I (761) PLAY_LIVING_STREAM: [ * ] Start and wait for Wi-Fi network
I (771) wifi:wifi driver task: 3ffd68bc, prio:23, stack:6656, core=0
I (771) system_api: Base MAC address is not set
I (771) system_api: read default base MAC address from EFUSE
I (781) wifi:wifi firmware version: c7d0450
I (781) wifi:wifi certification version: v7.0
I (781) wifi:config NVS flash: enabled
I (781) wifi:config nano formating: disabled
··················································
W (1591) PERIPH_WIFI: WiFi Event cb, Unhandle event_base:WIFI_EVENT, event_id:4
I (1591) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (2461) esp_netif_handlers: sta ip: 192.168.123.100, mask: 255.255.255.0, gw: 192.168.123.1
I (2461) PERIPH_WIFI: Got ip:192.168.123.100
I (2461) GET_TIME: Initializing SNTP
I (2471) GET_TIME: Waiting for system time to be set... (1/10)
I (2471) PLAY_LIVING_STREAM: [1.0] Create audio pipeline for playback
I (2481) PLAY_LIVING_STREAM: [1.1] Create http stream to read data
I (2491) PLAY_LIVING_STREAM: [2.2] Create PWM stream to write data to codec chip
I (2501) PLAY_LIVING_STREAM: [2.3] Create aac decoder to decode aac file
I (2501) PLAY_LIVING_STREAM: [2.4] Register all elements to audio pipeline
I (2511) PLAY_LIVING_STREAM: [2.5] Link it together http_stream-->aac_decoder-->pwm_stream-->[codec_chip]
I (2521) AUDIO_PIPELINE: link el->rb, el:0x3ffdceac, tag:http, rb:0x3ffdfa98
I (2531) AUDIO_PIPELINE: link el->rb, el:0x3ffdf718, tag:aac, rb:0x3ffdfad4
I (2541) PLAY_LIVING_STREAM: [2.6] Set up  uri (http as http_stream, aac as aac decoder, and default output is i2s(changed to PWM))
I (2551) PLAY_LIVING_STREAM: [ 3 ] Set up  event listener
I (2551) PLAY_LIVING_STREAM: [3.1] Listening event from all elements of pipeline
I (2561) PLAY_LIVING_STREAM: [3.2] Listening event from peripherals
I (2571) PLAY_LIVING_STREAM: [ 4 ] Start audio_pipeline
I (2581) AUDIO_ELEMENT: [http-0x3ffdceac] Element task created
I (2581) AUDIO_ELEMENT: [aac-0x3ffdf718] Element task created
I (2591) AUDIO_ELEMENT: [output-0x3ffdf338] Element task created
I (2601) AUDIO_PIPELINE: Func:audio_pipeline_run, Line:359, MEM Total:87268 Bytes

I (2601) AUDIO_ELEMENT: [http] AEL_MSG_CMD_RESUME,state:1
I (2611) AUDIO_ELEMENT: [aac] AEL_MSG_CMD_RESUME,state:1
I (2621) AUDIO_ELEMENT: [output] AEL_MSG_CMD_RESUME,state:1
I (2621) AUDIO_PIPELINE: Pipeline started
I (2631) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (2641) board_button: Button[0] created
I (2641) gpio: GPIO[2]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (2651) board_button: Button[1] created
I (2651) gpio: GPIO[3]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (2661) board_button: Button[2] created
I (2671) gpio: GPIO[6]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (2681) board_button: Button[3] created
I (2691) rda5807m: Device initialized
I (2691) rda5807m: Frequency: 102200 kHz
I (2701) HTTP_STREAM: total_bytes=789
I (2711) HTTP_STREAM: Live stream URI. Need to be fetched again!
I (2741) rda5807m: rda5807m_wait_and_finish_tune
I (2781) HTTP_STREAM: total_bytes=57780
I (2781) CODEC_ELEMENT_HELPER: The element is 0x3ffdf718. The reserve data 2 is 0x0.
I (2781) AAC_DECODER: a new song playing
I (2791) AAC_DECODER: this audio is RAW AAC
I (2821) PLAY_LIVING_STREAM: [ * ] Receive music info from aac decoder, sample_rates=24000, bits=16, ch=2
I (4251) GET_TIME: Notification of a time synchronization event
I (5471) main: The current date/time in Shanghai is: 2021-09-04      19:27:38
I (6471) main: The current date/time in Shanghai is: 2021-09-04      19:27:39
W (6801) HTTP_STREAM: No more data,errno:0, total_bytes:57780, rlen = 0
I (6801) AUDIO_ELEMENT: IN-[http] AEL_IO_DONE,0
I (6831) HTTP_STREAM: total_bytes=57848
```

**板子运行现象**

![12](/docs/images/12.jpg)

![13](/docs/images/13.jpg)

## 3 已知BUG

- SNTP校时任务可能会出现较长时间无法校时的情况
- WiFi输出日志中会出现`W (1591) PERIPH_WIFI: WiFi Event cb, Unhandle event_base:WIFI_EVENT, event_id:4`警告
- 按键无响应，只遇到过一次
- 网络电台切台过快程序会报错，目前只加延时简单处理

## 4 笔记

### 4.1 **项目前瞻**

刚刚看到这个[项目](https://www.eetree.cn/project/detail/419)的时候，下面详情中提供的参考资料全部都是使用Arduino实现的，所以我一开始觉得Arduino应该有很多轮子可以用，所以去乐鑫的Arduino的环境等等，确实，SSD1306、RDA5807M是有很好用的库，但是有一点就是关于音频的播放，都是使用额外的解码IC来完成的，但是项目提供的开发板并没有解码IC，若是自己编写mp3格式解码对我来说确实要花很多很多时间。所以又乐鑫的仓库寻找，发现了ESP-ADF这个好东西，里面有我需要的例程，所以才决定使用ESP-IDF来实现。（后面也发现Arduino可以作为IDF的组件来使用，但当时Arduino没有非常需要的库，所以只是试了一下demo就没用了）。

### 4.2 **环境搭建**

环境搭建在使用ESP-IDF中还是很重要的，当然没有环境肯定不能开发这是废话。重要的是在环境搭建过程中关于ESP-IDF整个框架的认识。在之前的使用ESP32制作一个接入HomeKit的空调控制器的项目中，因为没有用到很多第三方的库，所以只认识到了IDF框架的一点点。但是在这个项目中，因为要添加很多其他的库，需要对**组件**、**CMakeLists**、**Kconfig**、**Kconfig.projbuild**这些概念或文件有一定的了解。

如果需要添加其他组件到项目中，一方面可以在main下的Cmakelists文件中添加，或者直接拷贝库到components下，因为idf会自动搜索components文件夹下的组件。

而且每个组件必须包含Cmakelists文件来注册成为组件。一个库中包含很多组件，库的更目录中就会有一个`component.cmake`文件比如esp-iot-solution库中的`components\esp-iot-solution\component.cmake`，它会把库中的组件都包含进去。而且在每个组件中可以设置这个组件依赖于其他组件，所以很多时候如果set target失败了，八成是组件缺少了。

每个组件可以包含Kconfig文件来进行一定的设置，就像在**1.3 配置**中选项的那样，这些GUI的配置界面都是根据Kconfig文件里的内容来显示的，就像下面这样，选中某个选项就会生成一个以`CONFIG_`开头的宏定义，以此来控制条件编译，Kconfig.projbuild也是类似，一些例程进场会使用Kconfig.projbuild文件来配置WiFi名称密码这样的信息。这些事情都是因为遇到了上面LVGL默认字体不能改变的问题才学习到了这些东西。

<img src="/docs/images/9.png"  style="zoom: 80%;" />

### **4.3 程序思路**

上电进行WiFi和显示的初始化，在连接WiFi的时候加载一个Loading界面来等待，启动时间校准任务，启动网络电台任务，初始化按键，RDA5807M，进行主要界面GUI绘制。

时间校准是直接套用了IDF中SNTP的例程，也可以注册校准成功后的回调函数，校准成功后应该会自动更新RTOS中的时间，RTOS中会自动以校准后的时间为基准进行累加，所以后面显示的时候，直接读取RTOS的时间。如果超出了预期校准时间，那么RTOS时间好像是1970年开始，其中`struct tm `结构体类型中的`tm_year`这个成员，不是直接表示的年份，而是与1900年的差值。

LVGL创建了2个屏幕，一个负责网络电台的信息显示、另一个负责FM电台的信息显示。网络电台显示的有：IP地址、节目名称、音频信息、当前时间。FM电台显示的有：频段、RSSI、当前时间。两个屏幕同时存在，通过按键控制进行切换，切换时有个简单的滑动动画，程序可以设置动画时间，切换到另一个屏幕的时候，并没有删除另一个屏幕，仍然占据内存（也就几K），相当于作为后台运行。还给LVGL注册了输入设备轮询读取按键事件。

在运行过程中，首先会自动播放第一个网络电台，先获取m3u8文件后，对文件中的aac音频地址进行下载、解码、播放，获取解码播放这些功能在ADF中被称作元素，这些元素都被登记到一个管道中。

按键使用了iot_button组件，可以检测单击，双击，多击次数，长按这些状态，提供两种方式进行检测，轮询和回调，一开始使用轮询发现要么漏检，要么重复检测，所以后面改用了回调的方式，可以给每个按键设置对应状态的回调函数，项目中目前只用了单机和长按两种。因为LVGL读取按键是轮询的，所以在回调函数中记录当前按键的状态，LVGL读取状态，执行对应的操作后清除按键状态。模式切换和静音是直接在LVGL的读取按键的回调函数中进行的，单击模式切换的按键后，如果当前是网络电台屏幕，则停止网络电台管道切换至FM屏幕，同时创建读取FM信息任务。如果当前是FM屏幕，则删除读取FM信息任务，开始网络电台管道。节目切换或者频段切换是在标签对象的回调函数中进行的，初始化的时候设置了标签按键事件的回调函数，但是任何键值都会调用回调函数，所以需要在函数中判断键值，再根据当前屏幕进行相应的操作：切换网络电台或者频段。

RDA5807M库是在IDF库的基础上，又把IIC的库针对一些RDA5807M寄存器进行了封装，比如连续读模式和寄存器读模式。在RDA5807库依赖的`i2cdev`库，在每次进行IIC读写的时候都会设置IIC的地址和检测IIC是否进行初始化，设置IIC的地址可以理解，因为有两种模式的读写地址，但是不知道为什么都要检测IIC初始化，其中在读写的时候也用了互斥锁，应该是的吧，其中API中有带锁的读写和不带的。切到FM模式时，默认频率为102.2MHz，因为我这边102.2是有一个电台的，记得接上天线，一根杜邦线足以。按键每次改变0.1MHz。库中也添加了显示所有寄存器的值的函数和连续写模式的函数，这俩函数是为了解决下面4.4问题中使用的。

### 4.4 **遇到的问题**

在看IDF中的代码的时候，真的能察觉到世界的参差，那些代码写的是那么标准规范，自己写的代码真的烂了又烂。IDF都是基于FreeRTOS的，对FreeRTOS的接触也是从这里开始的，多线程的使用认识的很少，队列，锁也没有去使用。至少会了一点`xTaskCreate`和`vTaskDelete`.

LVGL

一：因为LVGL的ESP32库是针对IIC的SSD1306，却没有SPI的，esp-iot-solution中的LVGL有SPI的驱动却没有针对SSD1306的，所以我就把这两个例程缝合了起来，具体的细节都在[LVGL_PORT_ESP32_SSD1306-SPI](https://github.com/DaMiBear/LVGL_PORT_ESP32_SSD1306-SPI)这个的README中说了。而且这两个的例程的Kconfig文件还是有差距的，所以也有上面默认字体设置了却没作用这样的问题；二：还有就是LVGL的默认32K的全局变量的内存占用了，上面已经说明；三：还有第三点中文字体的问题，LVGL的内置字体`lv_font_simsun_16_cjk`缺了很多中文的字，所以参考了[MCU_Font_Release](https://gitee.com/WuBinCPP/MCU_Font_Release)来自定义字体；四：LVGL组对象中的聚焦问题，目前我这种程度的理解，LVGL中如果要使用输入设备，就必须把控制对象放到组中，再把组分配给输入设备。之后给LVGL提供一个轮询检测键盘的接口，进行相关按键操作。本人的思路是：获得某个键值后，启动组中某个对象的回调函数对该对象进行进一步的操作，但是问题是：组中的对象只有被聚焦的情况下，才有可能调用回调函数，不被聚焦是不可能调用回调函数的。在两个屏幕之间进行切换后，新的活动屏幕并不会主动去聚焦组中的对象，所以需要使用`lv_group_focus_obj(fre_label);`手动设置聚焦对象，才能获取到相关事件调用回调函数。

ADF

网络电台基本例程是`pipeline_living_stream`音频的控制参考`pipeline_sdcard_mp3_control`，项目中使用切换，或者是停止，都是只停止了一部分，整个ADF占用的资源并没有完全释放，因为尝试过删除设备等等，结果并没有成功。使用只使用了最简单的方法。还有蜻蜓FM的接入，看了文档不太理解，所以就仅仅对着URL中那段数字改，在蜻蜓FM中查早对应的电台名称。

RDA5807M

这个问题最要命，我一开始在esp-idf-lib中RDA5807M的例程基础上瞎改，比如把IIC的两个引脚调换、连续读写方式的地址和寄存器读写方式的地址调换。然后再改回最初的程序后，就没有声音输出了，在搜台模式下，频段也是一直卡在118.9MHz，一开始以为是设置的问题，结果把数据手册看了好几遍，也设置了好几遍，还是没有作用，同样的程序在别人的板子上就正常工作，后来没有办法了只能买了新的RDA5807M模块换上，结果一换就好了。而且后面还有一个坑，在更换了硬件之后，使用Arduino库来测试是正常工作的，但在使用esp-idf-lib中提供的例程，不论什么频段RSSI都是1，后来发现有一行程序把寄存器0x05写入了0x880f，在数据手册上看着是没问题，都是默认值，其中7:6位数据手册上没有写，但如果查看RDA5807MP的数据手册，可以发现7:6位是有定义的，RDA5807M和RDA5807MP不是一个芯片，但在RDA5807M的程序中按照RDA5807MP的手册设置了7:6位后，确实正常工作了。我也下了其他地方的RDA5807M的数据手册，都没有7:6位的定义。也许是我运气不好下的一直是劣质的数据手册？

<img src="/docs/images/10.png"  style="zoom:50%;" />

<img src="/docs/images/11.png"  style="zoom: 80%;" />

