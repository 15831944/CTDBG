 
## Win32-DLL型调试器,注入dll即可调试！


 _这个调试器是几年前调试游戏的时候做的，现在翻出来共享下。_ 

 代码基本上就是C代码,使用Vs编译 version >= 2010

 _过保护的功能只在Win7sp1 x64 上运行，有几处代码稍微补下现在也是可以用的 (config.h中开启)_ 

![输入图片说明](http://git.oschina.net/uploads/images/2016/1112/183732_e8f850a9_632350.png "在这里输入图片标题")

![输入图片说明](http://git.oschina.net/uploads/images/2016/1112/183736_f0248972_632350.png "在这里输入图片标题")

![输入图片说明](http://git.oschina.net/uploads/images/2016/1112/183742_0ba40c5b_632350.png "在这里输入图片标题")

除了界面内的操作，其他的快捷键分别是：
       
> 
> Ctrl+G:      //跳转到
> SPACE: //汇编此处
> F2:    //cc断点
> F3:    //条件cc断点
> F7:    //单步
> F9:    //运行
> 