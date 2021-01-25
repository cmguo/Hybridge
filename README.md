# Hybridge
统一的跨语言的对象间相互调用

## 支持语言：
* java（android）: [HybridgeJni](https://github.com/cmguo/HybridgeJni)
* dart（flutter）: [HybridgeDart](https://github.com/cmguo/HybridgeDart) base on [HybridgeC](https://github.com/cmguo/HybridgeC)
* c++（qt）: [HybridgeQt](https://github.com/cmguo/HybridgeQt)
* js（web）: [to be continued]
* objc (ios)： [to be continued]

## 支持的调用方式
* 跨语言访问（读、写）对象的属性
* 跨语言调用对象的方法
* 跨语言监听对象发出的信息（信号、事件）
* 跨语言监听对象属性值的变化

## 支持的部署方式
* 同一个进程内部，不同语言间相互调用

    这样的功能可以通过 jni、ffi 实现，然而使用 Hybridge 会让你的工作更简单，你完全不需要理解另一种语言，就可以将其他语言实现的对象完全当作本地对象使用。
    实际上 Hybridge 也是基于  jni、ffi 等机制实现的，只是提供了更好的封装。
    
* 同一台机器上，多个进程间相互调用

    利用本地操作系统提供的管道，你可以使用另一个进程的对象，类似 Android 的 Binder，但是 Hybridge 跨语言的特性会让你的方案更加灵活。

* 跨越网络，实现远程调用

    通过网络（比如 WebSocket），你可以远通过任何一个浏览器，来监控程序的运行状态，访问他的方法。然而你不需要为这些额外提供对象接口的封装，因为这些功能都是天然存在的了。
    
## 基本原理
* 基于 json 的消息格式，简单的对象访问协议
* 统一的底层逻辑（C++实现）
  * 管理类型元数据 （meta）
  * 管理对象实例、消息通道
  * 管理对象的属性状态，转发方法调用
  * 管理监听器
* 各种上层封装
  * java： 基于 jni 与 C++ 相互调用，使用 Proxy 自动实现接口
  * dart： 基于 ffi 与 C/C++ 相互调用，使用 source_gen 自动生成抽象类的实现
  * qt： 直接使用 qt 的元数据
  * js： 动态注册属性，方法

具体的使用方式，请参考各个语言的使用介绍。
