**[简体中文](皮肤制作教程) | English**

Since version 1.80, TrafficMonitor has added the support of skins in xml format. The skins in ini format in previous versions is still compatible.

This article focuses on xml skin making. If you want to see how to make the skins of old version, please refer to [旧版本的皮肤制作教程](旧版本的皮肤制作教程).

## Skin making Procedure

The skin files of TrafficMonitor is placed in the `skins` directory in the program directory. Each skin is placed in a separate folder. The folder name is the skin name.

To create a new skin, you need to create a folder named as the skin mane. Then put two background image files `background.bmp` and `background_l.bmp` in the folder.

Create a configuration file named as `skin.xml`, which is encoded as UTF8. You can config the displayed items for skin by editing this configuration file.

Please refer to the next section for the description of `skin.xml`.

## Skin configuration file in xml format

A typical skin configuration file in xml format is as following:

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

There are `skin`, `plugin_map`, `layout` and `preview` nodes under the root node of the xml file.

### skin node

The skin information is configured in the `skin` node.

* text_color

  Used to configure the text color of all items. The color value is the decimal value in `COLORREF` format, the format is `RRGGBB`. Each colors are separated with commas. The colors are listed in the following order: Upload, Download, CPU Usage, Memory usage, GPU usage, CPU temperature, GPU temperature, hard disk temperature, mainboard temperature, hard disk usage and plugin items.

* specify_each_item_color

  Whether to specify the different color for each item.

  If the value is 0, the first color in `text_color` will be the color of all items.

* skin_author

  The name of the skin maker.

* font

  Configure the font of the skin.

  The `name` and `size` attributes are used to configure the font name and font size of the skin.

  `style` is used to configure the font style of the skin. The first 4 bits of the integer is used for bold, italic, underline, and stripper styles respectively.

* display_text

  Configure the displayed label text for each item

  The meaning of the sub node name is as follow:

| Name of the sub node   | Meaning                |
| ---------------------- | ---------------------- |
| up                     | Upload speed           |
| down                   | Download speed         |
| total_speed            | Total speed            |
| cpu                    | CPU usage              |
| memory                 | Memory usage           |
| gpu                    | GPU usage              |
| cpu_temperature        | CPU temperature        |
| gpu_temperature        | GPU temperature        |
| hdd_temperature        | Hard disk temperature  |
| main_board_temperature | Main board temperature |
| hdd                    | Hard disk usage        |

### plugin_map node

TrafficMonitor allows the plugin items to displayed in the main window.

The `plugin_map` node is used to configure the mapping between the plugin item and the node name.

```xml
<time>ra1YX2g1</time>
<cosutom_item>b4zc373y</cosutom_item>
```

The node name is any specified plugin name, such as the `time` above. The value of the node is the plugin item ID, such as the `ra1YX2g1` above.

The plugin item ID can be find as following:

Open the context menu of TrafficMonitor, select "Other Functions" - "Plug-in Manage" (or select "Options" - "General Settings" - "Plugin-in Manage") to open the "Plugin Mange" dialog, select the plugin and click the "Details" button, the plugin item ID can be shown in the "Plugin details" dialog. As shown in the picture bellow.

<img src="./images/image52.jpg" style="zoom:80%;" />

If the plugin provides more then one display item, the ID of each item will be separated by a semicolon.

Tip: the text in the dialog can be copied by right click.

This node is not a must.

### layout node

The `layout` node is used to configure the size and position of all the items in the skin.

```xml
<layout text_height="20" no_label="0">
```

The attributes of the `layout` node are described as follow:

* text_height

  The height of all displayed item is the same. This attribute is use to configure the height of the displayed items. 

* no_label

  Used to configure whether this skin does not display label text.

  If this attribute is set as 1, when using this skin, the "Swap the position of upload and download" option in "Option Settings" - "Main Window Settings" will not be available.
  

There two sub nodes `layout_l`  and `layout_s` under the `layout` node. `layout_l` is used to configure the layout when "Show More Info" is checked, and `layout_s` is used to configure the layout when "Show More Info" is not checked.

The sub node name of `layout_l` or `layout_s` is the same as the sub node of `skin`/`display_text` mentioned above.

If you want to configure the display items for plugins, the node name must be configured in `plugin_map`.

There are several attributes in `layout_l` or `layout_s` nodes, which are used to configure the size of the main window.

```xml
<layout_l width="232" height="107">
```

* width

  The window width.

* height

  The window height.
  

When the skin is loaded, the size of the main window will be set to the size specified here. The size of the background image `background.bmp` and `background_l.bmp` dose not to be the same as value of `width` and `height` in  `layout_l` or `layout_s`, but the aspect ratio should be the same, otherwise the main window will be stretched out of shape.

The attribute of each sub node are described as following:

```xml
<cpu x="14" y="3" width="106" align="0" show="1"/>
```

* x

  The horizontal position of the display item in the window.

* y

  The vertical position of the display item in the window.

* width

  The width of the display item.

* align

  The alignment of the display item.

  0: align left

  1: align right

  2: align center

  3: align both side

  The effect is shown in the picture bellow:

  ![image-20220912181026156](images/image-20220912181026156.png)

  Align left ▲

  ![image-20220912181129041](images/image-20220912181129041.png)

  Align right ▲

  ![image-20220912181152487](images/image-20220912181152487.png)

  Align center ▲

  ![image-20220912181220356](images/image-20220912181220356.png)

  Align both side ▲

* show

  Whether this item is displayed in the main window.

### preview node

This note is used to configure the position of the skin in "Preview" of the "Change Skin" dialog.

The attributes `width` and `height` are used to configure the size of the preview area.

The position of the skin in preview area of checking and unchecking "Show More Info" is configured by the attribute `x` and `y` of the sub node `l` and `s`.

## Other Notes

* TrafficMonitor prefectly supports high DPI displays. All the values in the configuration file are the pixels when the DPI is set as 100%. If the DPI in your computer is set to more than 100%, the displayed item will be scaled according to the current DPI setting. Therefore, you do not need to change the value of any fields for different DPI settings.
* If a new skin is added, you need to restart TrafficMonitor to see the new skin in the "Change Skin" dialog.
* If you need to make irregular shape skin, please refer to [不规则形状的皮肤](旧版本的皮肤制作教程#不规则形状的皮肤).

