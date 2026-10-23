**简体中文 | [English](Plugin-Development-Guide)**

从1.82版本开始，TrafficMonitor支持插件系统，支持由用户自己开发插件。

插件可以用于在任务栏和主窗口中显示自定义项目，例如时间、日期、天气、股票等信息。

要使你编写的插件起作用，必须遵循本文所述的规范。

# 准备工作

要为TrafficMonitor开发插件，首先你必须要懂得如何编写C++程序，因为TrafficMonitor的插件提供的是C++的接口。

虽然TrafficMonitor使用了MFC开发，但是这并不意味着TrafficMonitor的插件也一定要使用MFC。TrafficMonitor插件的接口中不包含任何界面库的类，因此，理论上你可以使用任何基于C++的界面库为TrafficMonitor开发插件。

# 开发步骤

* 一切准备就绪后，你首先需要创建一个**动态库**项目。然后将[`PluginInterface.h`](https://github.com/zhongyang219/TrafficMonitor/blob/master/include/PluginInterface.h)这个C++头文件添加到你的项目中。这个文件非常重要，里面包含插件系统所有接口类的定义。

* 在你的项目中导出一个名为`TMPluginGetInstance`的函数，函数签名如下：

  ```c++
  ITMPlugin* TMPluginGetInstance();
  ```

  函数返回值为一个ITMPlugin接口的指针，参数列表为空。

  注意，此函数的实现中返回一个已存在对象的指针，不需要创建新的对象，框架也不负责释放此对象。

  示例代码如下：

  ```c++
  #ifdef __cplusplus
  extern "C" {
  #endif
      __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
  #ifdef __cplusplus
  }
  #endif
  
  /////////////////////////////////////////////////////////////
  ITMPlugin* TMPluginGetInstance()
  {
      return &CPluginDemo::Instance();
  }
  ```

* 编写一个类，继承自`ITMPlugin`接口，并实现接口的纯虚函数。此类的对象在内存中仅存在一个，由TMPluginGetInstance函数返回。

* 根据你的插件提供的显示项目，编写对应的类，继承自`IPluginItem`接口，并实现接口的纯虚函数。类的对象一般可作为`ITMPlugin`派生类的成员变量。例如，你的插件将提供时间和日期两个显示项目，则你需要编写两个类，它们都派生自`IPluginItem`接口，并将这两个类的对象作为`ITMPlugin`派生类的成员。

* 编译你的项目，将生成的dll文件复制到TrafficMonitor主程序所在目录下的`plugins`目录下，启动TrafficMonitor，插件将自动被加载。

  在TrafficMonitor右键菜单中选择“其他功能”——“插件管理”，在插件管理对话框中可以看到已加载的插件。如下图所示：
  
  <img src="images/image-20211112165246213.png" alt="image-20211112165246213" style="zoom:80%;" />

TrafficMonitor的源代码中提供了一个我写好的插件示例[PluginDemo](https://github.com/zhongyang219/TrafficMonitor/tree/master/PluginDemo)，它提供了时间和日期显示，以及一个自绘示例。插件的开发者可以以此示例为参考来开发TrafficMonitor的插件。

如果你使用MFC开发插件，可以点击以下链接下载插件模板：

[TrafficMonitorPlugins/PluginTemplate at main · zhongyang219/TrafficMonitorPlugins (github.com)](https://github.com/zhongyang219/TrafficMonitorPlugins/tree/main/PluginTemplate)

下载后请将文件名和文件内容中的“`PluginTemplate`”替换为你的插件名字，并根据提示添加你的代码。

如果你使用纯Win32API、Qt或者其他界面框架开发插件，则不适用此插件模板。

# 接口说明

## ITMPlugin接口

此接口为插件接口，派生自此接口的类必须重写必要的虚函数。

### GetAPIVersion

此函数返回接口的版本，仅当修改了插件接口时才会修改这里的返回值。插件开发者不应该修改这里的返回值，也不应该重写此虚函数。

### GetItem

此函数用于返回此插件提供的显示项目的对象。

**函数原型**

```c++
virtual IPluginItem* GetItem(int index) = 0;
```

**说明**

一个插件dll可以提供多个实现IPluginItem接口的对象，对应多个显示项目。此函数根据*index*的值返回对应的对象。

当*index*的值大于或等于0且小于IPluginItem接口的对象的个数时，返回对象的IPluginItem接口的指针，其他情况应该返回空指针。

**注意**

在实现此函数时请务必对index的值进行判断，切勿在未判断index的值的情况下直接返回对象的地址。

### DataRequired

**函数原型**

```c++
virtual void DataRequired() = 0;
```

**说明**

此函数由主程序每隔一定时间调用，插件需要在这里获取一次所有显示项目的数据。

### ShowOptionsDialog

**函数原型**

```c++
virtual OptionReturn ShowOptionsDialog(void* hParent);
```

**说明**

此函数不一定要重写。如果插件提供了选项设置界面，则应该重写此函数，并在最后返回OR_OPTION_CHANGED或OR_OPTION_UNCHANGED。

*hParent*

选项设置对话框的父窗口的句柄。

*返回值*

返回一个枚举值。

| 枚举项                 | 说明                           |
| ---------------------- | ------------------------------ |
| OR_OPTION_CHANGED      | 选项设置对话框中更改了选项设置 |
| OR_OPTION_UNCHANGED    | 选项设置对话框中未更改选项设置 |
| OR_OPTION_NOT_PROVIDED | 未提供选项设置对话框           |

如果在派生类中重写了此函数，则必须返回OR_OPTION_CHANGED或OR_OPTION_UNCHANGED，不能返回OR_OPTION_NOT_PROVIDED。

当用户在主程序的“插件管理”对话框中点击“选项”按钮时会调用此函数。如下图所示：

![image-20211112195145679](images/image-20211112195145679.png)

如果函数返回OR_OPTION_NOT_PROVIDED，则点击“选项”按钮时会提示“此插件没有提供选项设置”。

### GetInfo

**函数原型**

```c++
virtual const wchar_t* GetInfo(PluginInfoIndex index) = 0;
```

**说明**

主程序调用此函数以获取此插件的信息。根据*index*的值返回对应的信息。

index的值请参见PluginInfoIndex类型的注释。

通过此函数返回的信息将显示在“插件管理”——“详细信息”中，如下图所示：

![image-20211112194635969](images/image-20211112194635969.png)

注意：请勿返回空指针。

### OnMonitorInfo

**函数原型**

```c++
virtual void OnMonitorInfo(const MonitorInfo& monitor_info);
```

**说明**

此函数不一定要重写。主程序在获取到监控信息时，会调用此函数以向插件传递所有获取到的监控信息。这就意味着，在插件中也可以获取到主程序中取得的例如网速、CPU和内存利用率、温度等信息。

*monitor_info*

MonitorInfo结构的对象，包含了主程序中取得的所有监控信息。

### GetTooltipInfo

**函数原型**

```c++
virtual const wchar_t* GetTooltipInfo();
```

**说明**

此函数不一定要重写。函数返回用于显示在鼠标提示中的文本，如果返回空字符串，则不在鼠标提示中添加任何文本。

注意：请勿返回空指针。

### OnExtenedInfo

**函数原型**

```c++
virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data);
```

**说明**

此函数不一定要重写。主程序会此需要的时候调用此函数以向插件传递更多信息，重写此函数可以接收主程序传递的信息。

*index*

信息的索引，用于区分向插件传递的信息。它在ExtendedInfoIndex枚举中定义。随着接口的更新，ExtendedInfoIndex枚举可能会增加更多的枚举项。

*data*

传递的数据，用字符串表示。

## IPluginItem接口

此接口为插件显示项目接口，你需要为你的插件提供的每一个显示项目编写对应的类，并实现此接口。

### GetItemName

**函数原型**

```c++
virtual const wchar_t* GetItemName() const = 0;
```

**说明**

返回插件项目的名称。这个名称将显示在TrafficMonitor任务栏右键菜单“显示项目”子菜单下，以及选项设置对话框中的“主窗口设置”和“任务栏窗口设置”中的“显示设置”对话框和文本颜色设置对话框中，如下图所示：

<img src="images/image-20211112165501363.png" alt="image-20211112165501363" style="zoom:80%;" />

注意：请勿返回空指针。

### GetItemId

**函数原型**

```c++
virtual const wchar_t* GetItemId() const = 0;
```

**说明**

函数返回一个字符串，用作显示项目的唯一ID，用于区分每个显示项目。应该确保此ID不会和其他插件显示项目重复。ID应该只包含字母和数字。

请勿返回空指针。

### GetItemLableText

**函数原型**

```c++
virtual const wchar_t* GetItemLableText() const = 0;
```

**说明**

函数返回一个字符串，作为显示项目的标签文本。 

如果IsCustomDraw函数返回true，则这里的返回值将会被忽略。

请勿返回空指针。

### GetItemValueText

**函数原型**

```c++
virtual const wchar_t* GetItemValueText() const = 0;
```

**说明**

函数返回一个字符串，作为显示项目的数值的文本。

如果IsCustomDraw函数返回true，则这里的返回值将会被忽略。

请勿返回空指针。

**注意**

由于此函数可能会被频繁调用，因此不要在这里获取监控数据，而是在ITMPlugin::DataRequired函数中获取。在这里返回获取到的用于显示的字符串格式的数值。

### GetItemValueSampleText

**函数原型**

```c++
virtual const wchar_t* GetItemValueSampleText() const = 0;
```

**说明**

函数返回一个字符串，作为显示项目数值的示例文本。这里返回的文本会用作“更换皮肤”对话框中预览图中显示的文本，同时也用于根据此字符串的长度计算任务栏显示项目的宽度。

如果IsCustomDraw函数返回true，则这里的返回值将会被忽略。

请勿返回空指针。

### IsCustomDraw

**函数原型**

```c++
virtual bool IsCustomDraw() const;
```

**说明**

此函数仅需返回true或false即可。用于设置此显示项目的显示区域是否由插件自行绘制。

如果此函数返回false，则此显示项目不由插件自行绘制，而是由主程序绘制。主程序将根据GetItemLableText和GetItemValueText返回的文本绘制显示区域，此时重写DrawItem函数将不起作用。

如果此函数返回true，则显示项目由插件自行绘制。需要绘制显示区域时，则主程序调用DrawItem函数进行绘制。此时必须重写DrawItem函数并在里面添加绘制显示区域的代码，此时GetItemLableText、GetItemValueText和GetItemValueSampleText的返回值将被主程序忽略。

### GetItemWidth

**函数原型**

```c++
virtual int GetItemWidth() const
```

**说明**

此函数用于获取显示区域的宽度。只有当IsCustomDraw函数返回true时重写此函数才有效。

**注意**

* 返回的值为DPI为96（100%）时的宽度，主程序会根据当前系统DPI的设置自动按比例放大，因此你不需要为不同的DPI设置返回不同的值。
* 这里的返回值代表了自绘区域所需要的最小宽度，DrawItem函数中的参数w的值可能会大于这个值

### GetItemWidthEx

**函数原型**

```c++
virtual int GetItemWidthEx(void* hDC) const;
```

**说明**

此函数的作用和GetItemWidth一样，为了兼容旧版本的插件，在保留了GetItemWidth函数的情况下添加了此函数。和GetItemWidth函数相比，此函数增加了参数*hDC*，它是绘图的上下文句柄，可以根据它来计算显示区域的宽度。

主程序在需要获取显示项目的宽度时，会先调用此函数，如果返回值为0，再调用GetItemWidth函数。

### DrawItem

**函数原型**

```c++
virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
```

**说明**

重写此函数并添加绘图的代码，用于自定义绘制显示区域，只有当CustomDraw函数返回true时重写此函数才有效。

*hDC*

绘图的上下文句柄。

*x, y, w, h*

绘图的矩形区域。

*dark_mode*

是否为深色模式。

### OnMouseEvent

**函数原型**

```c++
virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag);
```

**说明**

当插件显示区域有鼠标事件时由主程序调用。

*type*

MouseEventType类型的枚举项，表示鼠标事件的类型。

#### MouseEventType

| 枚举项       | 说明           |
| ------------ | -------------- |
| MT_LCLICKED  | 点击了鼠标左键 |
| MT_RCLICKED  | 点击了鼠标右键 |
| MT_DBCLICKED | 双击了鼠标左键 |

*x, y*

鼠标指针所在的坐标。此坐标是基于任务栏窗口或主窗口左上角为原点的坐标。

*hWnd*

产生此鼠标事件的窗口的句柄（主窗口或任务栏窗口）。

*flag*

若干MouseEventFlag枚举常量的组合。

#### MouseEventFlag

| 枚举项         | 说明                       |
| -------------- | -------------------------- |
| MF_TASKBAR_WND | 是否为任务栏窗口的鼠标事件 |

*返回值*

如果返回1，则主程序认为插件已经对此鼠标事件作出了全部的响应，主程序将不会再对此鼠标事件做额外的响应。
例如当type为MT_RCLICKED时，如果程序返回0，则会弹出主程序提供的右键菜单；而返回1时，主程序不会再做任何处理。

## 示例代码

### CPluginDemo类（继承自ITMPlugin接口）

[PluginDemo.h](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginDemo.h)

[PluginDemo.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginDemo.cpp)

### CPluginSystemTime类、CPluginSystemTime类（继承自IPluginItem接口）

[PluginSystemTime.h](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginSystemTime.h)

[PluginSystemTime.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginSystemTime.cpp)

[PluginSystemDate.h](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginSystemDate.h)

[PluginSystemDate.cpp ](https://github.com/zhongyang219/TrafficMonitor/blob/master/PluginDemo/PluginSystemDate.cpp)

## 插件的调试

现在已经提供了一个插件测试工具，存放在[TrafficMonitorPlugins](https://github.com/zhongyang219/TrafficMonitorPlugins)仓库中。

在开发插件时，你只需要将你的插件项目和PluginTester项目放在同一个解决方案中，并且将插件dll和PluginTester.exe生成到同一目录中，然后将PluginTester设置为活动项目，按下F5键即可调试。

关于插件测试工具的使用说明请参考以下链接：

[插件测试器 · zhongyang219/TrafficMonitorPlugins Wiki (github.com)](https://github.com/zhongyang219/TrafficMonitorPlugins/wiki/插件测试器)

需要注意的是，插件测试工具采用了和TrafficMonitor主程序同样的方法加载插件，但是你的插件在插件测试工具中工作正常并不能代表它们在TrafficMonitor中也能正常工作。插件开发完成之后请务必使用TrafficMonitor对你的插件做进一步的测试。
