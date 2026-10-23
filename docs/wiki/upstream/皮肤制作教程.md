**简体中文 | [English](Skin-Making-up-Tutorial)**

从1.80版本开始，TrafficMonitor新增了xml格式的皮肤。仍然兼容以前版本的ini格式的皮肤。

本文主要介绍xml格式皮肤的制作，如果你想查看关于如何制作旧版本的皮肤，请参考“[旧版本的皮肤制作教程](旧版本的皮肤制作教程)”。

## 皮肤制作步骤

TrafficMonitor的皮肤文件放在主程序所在目录的`skins`目录下，每个皮肤被放到单独的文件夹下，文件夹的名字就是皮肤的名称。

要创建一个新的皮肤，需要创建一个以皮肤名称命名的文件夹，然后制作制作好`background.bmp`和`background_l.bmp`两个背景图片放在文件夹下。

创建一个名为“`skin.xml`”的配置文件，文件编码为UTF8。通过编辑此配置文件来配置皮肤中包含的显示项目。

关于`skin.xml`的说明请见下一小节。

## xml格式的皮肤配置文件

一个典型的xml格式的皮肤配置文件如下：

```xml
<root>
	<skin>
		<text_color>16768959,10022341,16777215,16777215,16777215,16770992,16770992,16770992,16770992</text_color>
		<specify_each_item_color>1</specify_each_item_color>
		<skin_author>zy</skin_author>
		<font name="Microsoft YaHei" size="9" style="0"/>
		<display_text>
			<up>上传: </up>
			<down>下载: </down>
			<cpu>CPU: </cpu>
			<memory>内存: </memory>
			<gpu>显卡: </gpu>
			<cpu_temperature>CPU温度: </cpu_temperature>
			<gpu_temperature>显卡温度: </gpu_temperature>
		</display_text>
	</skin>
	<plugin_map>
		<time>ra1YX2g1</time>
		<cosutom_item>b4zc373y</cosutom_item>
	</plugin_map>
	<layout text_height="20" no_label="0">
		<layout_l width="232" height="107">
			<cpu x="14" y="3" width="106" align="0" show="1"/>
			<memory x="88" y="3" width="106" align="0" show="1"/>
			<gpu x="162" y="3" width="106" align="0" show="1"/>
			<up x="14" y="27" width="106" align="0" show="1"/>
			<down x="122" y="27" width="106" align="0" show="1"/>
			<cpu_temperature x="32" y="57" width="94" align="0" show="1"/>
			<gpu_temperature x="32" y="83" width="94" align="0" show="1"/>
			<time x="142" y="57" width="94" align="0" show="1"/>
			<cosutom_item x="142" y="83" width="94" align="0" show="1"/>
		</layout_l>
		<layout_s width="232" height="50">
			<cpu x="14" y="3" width="106" align="0" show="1"/>
			<memory x="88" y="3" width="106" align="0" show="1"/>
			<gpu x="162" y="3" width="106" align="0" show="1"/>
			<up x="14" y="27" width="106" align="0" show="1"/>
			<down x="122" y="27" width="106" align="0" show="1"/>
		</layout_s>

	</layout>
	<preview width="250" height="170">
		<l x="0" y="0"/>
		<s x="0" y="112"/>
	</preview>
</root>
```

在xml的根节点下共有`skin`、`plugin_map`、`layout`和`preview`节点。

### skin节点

skin节点下配置了皮肤的相关信息。

* text_color

  用于配置所有项目的文本颜色，颜色值为COLORREF格式的十进制值，以RRGGBB的格式。每个颜色使用逗号隔开。顺序按照“上传”、“下载”、“CPU利用率”、“内存利用率”、“显卡利用率”、“CPU温度”、“显卡温度”、“硬盘温度”、“主板温度”、“硬盘利用率”，以及插件项目的顺序排列。

* specify_each_item_color

  是否为每个项目指定不同的颜色。

  如果为0则会将text_color中的第一个颜色作为所有项目的颜色。

* skin_author

  配置皮肤制作者的名字。

* font

  配置皮肤的字体。

  name和size属性用于配置皮肤的字体名称和字体大小。

  style用于配置皮肤字体样式。使用整数的前4个bit位分别表示加粗、斜体、下划线和删除线样式。

* display_text

  用于配置每个项目的标签显示文本。

  其中子节点的名称含义如下：

| 子节点名称             | 含义       |
| ---------------------- | ---------- |
| up                     | 上传速度   |
| down                   | 下载速度   |
| total_speed            | 总网速     |
| cpu                    | CPU利用率  |
| memory                 | 内存利用率 |
| gpu                    | 显卡利用率 |
| cpu_temperature        | CPU温度    |
| gpu_temperature        | 显卡温度   |
| hdd_temperature        | 硬盘温度   |
| main_board_temperature | 主板温度   |
| hdd                    | 硬盘利用率 |

### plugin_map节点

TrafficMonitor支持在主窗口显示插件项目。

plugin_map节点用于配置插件项目和节点名称的对应关系。

```xml
<time>ra1YX2g1</time>
<cosutom_item>b4zc373y</cosutom_item>
```

其中节点名称为任意指定的插件名称，如上面的`time`所示；节点的值为插件的ID，如上面的`ra1YX2g1`所示。

插件的ID可以通过以下方式查询：

打开TrafficMonitor的右键菜单——“其他功能”——“插件管理”（或者通过“选项设置”——“常规设置”——“插件管理”）打开插件管理对话框，选中对应的插件，点击“详细信息”，即可查看插件的ID。如下图所示：

<img src="./images/image52.jpg" style="zoom:80%;" />

如果插件提供了多个显示项目，则每个项目的ID将使用分号隔开。

提示：使用鼠标右键可以复制文本。

此节点不是必须的。

### layout节点

layout节点用于配置皮肤中所有项目的大小和位置。

```xml
<layout text_height="20" no_label="0">
```

layout节点的属性说明如下：

* text_height

  所有显示项目的高度都是一样的，此属性用于配置显示项目的高度。

* no_label

  用于配置此皮肤是否不显示标签文本。

  如果属性设置为1，则使用此皮肤时，“选项设置”——“主窗口设置”中的“显示文本设置”和“交换上传和下载的位置”选项将不可用。

layout节点下面有两个子节点——`layout_l`和`layout_s`。其中`layout_l`用于配置勾选了“显示更多信息”时的布局，而`layout_s`用于配置不勾选“显示更多信息”时的布局。

`layout_l`或`layout_s`子节点的名称的含义同上述`skin`/`display_text`子节点的名称。

如果要为插件配置显示项目，节点的名称必须在`plugin_map`中配置。

`layout_l`或`layout_s`节点有若干属性，用于配置主窗口的大小。

```xml
<layout_l width="232" height="107">
```

* width

  窗口的宽度

* height

  窗口的高度
  

在加载皮肤时，主窗口会被设置成这里设置的大小。背景图片`background.bmp`和`background_l.bmp`的尺寸不需要和`layout_l`或`layout_s`中width和height的值一致，但是长宽比应该一致，否则会导致拉伸变形。

 每个子节点的属性说明如下：

```xml
<cpu x="14" y="3" width="106" align="0" show="1"/>
```

* x

  显示项目在窗口中的水平起始位置。

* y

  显示项目在窗口中的垂直起始位置。

* width

  显示项目的宽度。

* align

  显示项目文本的对齐方式。

  其中：

  0：左对齐；1：右对齐；2：居中；3：两端对齐

  效果如图所示：

  ![image-20220912181026156](images/image-20220912181026156.png)

  左对齐▲

  ![image-20220912181129041](images/image-20220912181129041.png)

  右对齐▲

  ![image-20220912181152487](images/image-20220912181152487.png)

  居中▲

  ![image-20220912181220356](images/image-20220912181220356.png)

  两端对齐▲

* show

  此项目是否在主窗口中显示。

### preview节点

此节点用于配置皮肤在“更换皮肤”对话框中“预览”界面的位置。

preview节点的属性`width`和`height`分别用于配置“预览”界面的大小。

子节点`l`和`s`分别通过属性`x`和`y`配置勾选“显示更多信息”和不勾选“显示更多信息”时的布局在“预览”界面中的位置。

## 其他说明

* TrafficMonitor完美支持高DPI显示器，配置文件中所有数值的单位均为DPI设定为100%时的像素值，如果你的电脑的DPI设置大于100%，在显示时所有数据会根据DPI进行等比例缩放，因此，你不需要为不同的DPI设置而修改任何字段的数值。
* 如果新增了皮肤，需要重新启动软件才能在“更换皮肤”界面中看到新增的皮肤。
* 如果需要制作不规则形状皮肤，请参考“[不规则形状的皮肤](旧版本的皮肤制作教程#不规则形状的皮肤)”。

