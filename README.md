# GRSWindowsSamples
# 1、简介（Introduction）

&emsp;&emsp;除了GUI GDI之外，这里囊括了几乎所有的 Windows 功能的基础示例，这样做，首先是为了方便各位了解关于Windows 的所有基本 API；其次是为了方便各位在设计游戏引擎时对除了 DirectX 之外的功能有一个比较完整的功能参考列表，比如：多线程、多进程、内存管理、线程同步、文件系统、网络、XML等等，虽然这些功能都是 Windows 版的，但是现在几乎所有的操作系统都有这些对应的功能，并且在细节上也都很类似，因此在进行抽象设计时这也是一套不错的参考手册。

&emsp;&emsp;With the exception of GUI GDI, I've included basic examples of almost all of Windows's features, first of all to help you understand all the basic apis of Windows; Secondly, in order to facilitate you in the design of the game engine in addition to DirectX features have a more complete functional reference list, such as: Multithreading, multi-processes, memory management, thread synchronization, file systems, networking, XML, etc., all of these features are available for Windows, but almost all operating systems now have them and are similar in detail, so this is a good reference manual for abstract design.

# 2、来源（Source）

&emsp;&emsp;这套示例来自本人10多年为视频教程准备的示例，目前视频教程被别人发在了B站：[C++起步到MFC实战VC++软件工程师高端培训(服务器端开发方向)](https://www.bilibili.com/video/BV187411s7dH/?spm_id_from=333.999.0.0) 视频教程已经年代久远了，仅供各位有兴趣的可以去看看，不想看的就直接看这里的示例学习即可。

&emsp;&emsp;This set of examples is from the example I prepared for the video tutorial for more than 10 years, and the video tutorial has been posted on the B website by others: [start to c + + MFC practical vc + + software engineer high-end training (server-side development direction)](https://www.bilibili.com/video/BV187411s7dH/?spm_id_from=333.999.0.0) video tutorials have been old, Just for you interested can go to see, do not want to see the example can be directly read here.

# 3、代码组织形式（Code organization）

&emsp;&emsp;本套代码一共212个示例，被按顺序分成了 29 个部分，并且全部按照双字母排序进行了项目命名，使用VS2022打开后，切换到解决方案资源管理器后就可以看到 29 个部分及相关示例。每个示例都做了最简化处理，仅展示本范围内的一个主题，并且没有引用任何第三方库或类，所有的示例几乎都是 C风格的代码，方便各位学习和掌握。代码大部分都参考了 MSDN 本身的示例，但是做了相应的修改和简化，为了使更好的掌握这些基本功能。

&emsp;&emsp;There are 212 examples in this code, divided into 29 parts in order, and all of them are named in double-alphabetical order. After opening with VS2022, you can see the 29 parts and related examples when you switch to Solution Explorer. Each example has been simplified to show only one topic in this scope and does not reference any third party libraries or classes. All of the examples are almost C-style code for easy learning and mastery. Most of the code references MSDN's own examples, but has been modified and simplified in order to better grasp these basic functions.

# 4、重要提示（Important note）

&emsp;&emsp;本套代码中有些示例代码有潜在风险，比如：DLL 注入示例、跨进程内存分配查询、磁盘扇区读写、LSP（Winsock服务提供者接口 SPI）示例、系统未公开API调用、Windows 权限提升、注册表操作等示例，运行后可能会引起系统故障，所以请各位务必在搞懂示例原理的基础上再去试运行，有条件的请在虚拟机中学习编译调试这些示例，以免引起不必要的麻烦。

&emsp;&emsp;另外示例中有些内容需要有对应的硬件环境才能运行，比如：COM端口读写示例、磁带的读写等。所以没有这些硬件条件的情况下是没法运行的，请不要误解为示例有问题。

&emsp;&emsp;本人对各位因贸然编译运行这些示例而引起的系统损坏或其它损失概不负责。

&emsp;&emsp;Some of the examples in this code are potentially risky, such as: DLL injection example, cross-process memory allocation query, disk sector read and write, LSP (Winsock Service provider Interface SPI) example, system unpublished API call, Windows permission promotion, registry operation and other examples may cause system failure after running. Therefore, please be sure to understand the principle of the example on the basis of trial run, conditional please learn to compile and debug these examples in the virtual machine, so as not to cause unnecessary trouble.

&emsp;&emsp;&emsp;In addition, some content in the example requires the corresponding hardware environment to run, such as COM port read and write examples and tape read and write examples. Therefore, it cannot run without these hardware conditions. Please do not misinterpret this as a problem with the example.

&emsp; &emsp; I am not responsible for any system damage or other damage caused by you rushing to compile and run these examples.

# 5、未来计划（Future plan）

&emsp;&emsp;这套示例代码将不定期的进行更新，将来可能更新的重点范围在：多线程、内存管理、文件管理、同步对象、网络等比较基础重要的部分。其它的部分因为有些已经太古老或者是有更好的替代库就不再更新了，保留仅供各位参考用。

&emsp;&emsp;另外因为这套代码年代久远，目前全部都是Win32 32配置形式，没有配置x64的编译配置，并且部分示例可能在VS2022下编译会出现问题，这些问题都将在未来逐步修正，因为有些示例在x64环境下需要做些修改。现在仅是上传提交初始化。

&emsp;&emsp;未来在展示关于游戏引擎封装的模块、思路和方法的时候，这套示例都将是最重要的参考部分之一，所以有兴趣的网友请务必仔细看下重点的部分，以免在看我后续的教程的时候，有什么知识空白。

&emsp;&emsp;This sample code will be updated from time to time, and future updates may focus on: multithreading, memory management, file management, synchronization objects, network and other more basic and important parts. Other sections are not updated because some are too old or have better alternatives, and are reserved for your reference only.

&emsp;&emsp;In addition, due to the age of this code, all the current Win32 32 configuration form, no configuration of x64 compilation configuration, and some examples may be compiled under VS2022 problems, these problems will be gradually corrected in the future, because some examples need to be modified under x64 environment. Now it's just upload commit initialization.

&emsp;&emsp;This set of examples will be one of the most important references in future presentations of several modules, ideas, and methods encapsulated in game engines, so please take a close look at the highlights to avoid any gaps in my subsequent tutorials.

# 6、协议（LICENSE）

&emsp;&emsp;因为这套示例库，基本没有什么封装，仅仅是为了展示 Windows API的基本调用方法和功能的，所以也不具备直接使用的价值，只有学习参考备忘的价值。最终使用 MIT 协议公开。

&emsp;&emsp;Because this sample library, basically no packaging, just to show the Windows API basic call methods and functions, so it does not have the value of direct use, only to learn the reference memo value. It was eventually exposed using the MIT protocol.



 
