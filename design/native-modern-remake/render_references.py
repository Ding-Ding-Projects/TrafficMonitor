"""Generate editable design proposals, never application evidence."""
from pathlib import Path
from html import escape

ROOT = Path(__file__).parent / "references"
ROOT.mkdir(exist_ok=True)
PALETTES = {
    "light": dict(bg="#F5F7FA", panel="#FFFFFF", nav="#EDF1F6", text="#17212F", muted="#526174", line="#D5DEE8", accent="#245FC8", selected="#DCE8FF", ink="#FFFFFF"),
    "dark": dict(bg="#101419", panel="#1B2129", nav="#151B23", text="#E6EDF5", muted="#A8B7C8", line="#394758", accent="#91B8FF", selected="#243D63", ink="#101419"),
}
SIZES = {"workspace": ((1280,840),(800,600)), "settings": ((1280,840),(760,640)), "setup": ((960,760),(760,640)), "menu": ((420,480),(320,400)), "regex": ((1280,840),(640,400))}

def render(surface, size, lang, theme, state="initial"):
    w,h = SIZES[surface][size == "minimum"]
    p = PALETTES[theme]
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}"><title>{surface} {size} {lang} {theme} design proposal</title><desc>Authored design reference with synthetic values. Not a running application or screenshot.</desc>']
    def box(x,y,bw,bh,fill="panel",r=0,stroke=None):
        out.append(f'<rect x="{x}" y="{y}" width="{bw}" height="{bh}" rx="{r}" fill="{p.get(fill,fill)}"'+(f' stroke="{p[stroke]}"' if stroke else '')+'/>')
    def line(x,y,x2,y2):
        out.append(f'<path d="M{x} {y}H{x2}" stroke="{p["line"]}"/>' if y==y2 else f'<path d="M{x} {y}L{x2} {y2}" stroke="{p["line"]}"/>')
    def text(x,y,en,zh="",sz=14,color="text",weight=400):
        values = [(en,sz)] if lang=="english" or not zh else [(zh,sz)] if lang=="cantonese" else [(en,sz),(zh,max(12,sz-2))]
        for i,(value,fs) in enumerate(values):
            out.append(f'<text x="{x}" y="{y+i*(sz+5)}" fill="{p[color]}" font-family="Segoe UI,Microsoft JhengHei,sans-serif" font-size="{fs}" font-weight="{weight}">{escape(value)}</text>')
    ch = 48 if lang=="bilingual" else 36
    def button(x,y,bw,en,zh="",primary=False,plain=False):
        if not plain:box(x,y,bw,ch,"accent" if primary else "selected",12,None)
        text(x+12,y+21,en,zh,14,"accent" if plain else "ink" if primary else "text",500)
    def field(x,y,bw,en,zh="",value="",selected=False):
        box(x,y,bw,ch+16,"selected" if selected else "nav",8,None)
        out.append(f'<path d="M{x+8} {y+ch+16}H{x+bw-8}" stroke="{p["accent" if selected else "line"]}" stroke-width="{2 if selected else 1}"/>')
        text(x+12,y+17,en,zh,12,"muted")
        text(x+12,y+ch+6,value,sz=14)
    def scroll(x,y,bh,content,position=0):
        if content<=bh:return
        thumb=min(bh,max(32,bh*bh/content))
        offset=(bh-thumb)*min(max(position,0),content-bh)/(content-bh)
        box(x,y,8,bh,"nav",4);box(x+1,y+offset,6,thumb,"muted",3)
    def search(x,y,bw):
        placeholder=("尋找設定" if lang=="cantonese" else "Find a setting" if lang=="english" else "Find a setting / 尋找設定") if surface=="settings" else ""
        field(x,y,bw-76,"Search","搜尋",placeholder)
        button(x+bw-68,y+8,64,"Regex","正則")
    def nav(items,selected=0):
        box(0,0,176,h,"nav");text(20,34,"TrafficMonitor",sz=17,weight=600)
        for i,(en,zh) in enumerate(items):
            yy=72+i*(58 if lang=="bilingual" else 46)
            if i==selected:
                box(12,yy-22,152,ch+4,"selected",12)
                box(12,yy-12,3,ch-12,"accent",2)
            ic=p["accent" if i==selected else "muted"]
            # Authored semantic line icons supplement the full accessible labels.
            icons={
                "Monitoring":'<rect width="16" height="13" rx="2"/><path d="M3 9l3 -4 3 2 4 -4M5 16h6M8 13v3"/>',
                "Monitor":'<rect width="16" height="13" rx="2"/><path d="M3 9l3 -4 3 2 4 -4M5 16h6M8 13v3"/>',
                "Floating monitor":'<rect x="1" y="1" width="14" height="14" rx="2"/><path d="M1 5h14M9 9h4M9 12h4"/>',
                "Taskbar":'<rect x="1" y="1" width="14" height="14" rx="2"/><path d="M1 11h14M4 13h2M8 13h4"/>',
                "Notifications":'<path d="M3 11V7a5 5 0 0110 0v4l2 2H1zM6 15h4"/>',
                "Interface":'<circle cx="8" cy="8" r="7"/><path d="M8 1v14M8 1a7 7 0 010 14"/>',
                "All groups":'<rect x="1" y="1" width="5" height="5" rx="1"/><rect x="10" y="1" width="5" height="5" rx="1"/><rect x="1" y="10" width="5" height="5" rx="1"/><rect x="10" y="10" width="5" height="5" rx="1"/>',
                "History":'<circle cx="8" cy="8" r="7"/><path d="M8 3v5l3 2"/>',
                "Plugins":'<path d="M2 3h4V1h4v2h4v4h2v4h-2v4H2v-4h3V7H2z"/>',
                "Settings":'<circle cx="8" cy="8" r="5"/><circle cx="8" cy="8" r="2"/><path d="M8 0v3M8 13v3M0 8h3M13 8h3M2 2l2 2M12 12l2 2M2 14l2 -2M12 4l2 -2"/>',
                "Setup":'<path d="M2 4h12M2 12h12"/><circle cx="6" cy="4" r="2"/><circle cx="10" cy="12" r="2"/>',
            }
            icon=icons.get(en,'<circle cx="8" cy="8" r="7"/>')
            out.append(f'<g transform="translate(24 {yy-13})" fill="none" stroke="{ic}" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">{icon}</g>')
            text(50,yy,en,zh,14,"accent" if i==selected else "muted",600 if i==selected else 400)
        text(20,h-36,"Native monitor","原生監察",12,"muted")
    box(0,0,w,h,"bg")
    if surface in ("settings","setup"):
        if surface=="settings":
            nav([("Monitoring","監察"),("Floating monitor","浮動監察器"),("Taskbar","工作列"),("Notifications","通知"),("Interface","介面"),("All groups","所有組別")],4)
        else:nav([("1  Welcome","1  開始設定"),("2  Monitoring","2  監察選項"),("3  Startup","3  啟動選項"),("4  Review","4  檢查設定")],0)
        x=200;cw=min(860,w-x-24)
        text(x,38,"Settings" if surface=="settings" else "Set up your monitor","設定" if surface=="settings" else "設定監察工具",24,weight=600)
        sy=82 if lang=="bilingual" else 62
        search(x,sy,cw)
        fy=h-(114 if lang=="bilingual" else 80)
        top=sy+ch+28;view=fy-top-16
        wide=cw>=600
        label_height=39 if lang=="bilingual" else 20
        help_height=35 if lang=="bilingual" else 18
        row_height=max(label_height+8+help_height,ch)+24 if wide else label_height+8+ch+24
        content=24+(48 if lang=="bilingual" else 36)+2*row_height+80+24
        if surface=="setup":content+=ch+32+2*(label_height+44)+24
        offset=max(0,content-view-32) if state=="scrolled" else 0
        box(x-12,top,cw+8,min(view,content),"panel",16)
        out.append(f'<defs><clipPath id="form"><rect x="{x-12}" y="{top}" width="{cw+8}" height="{view}"/></clipPath></defs><g clip-path="url(#form)"><g transform="translate(0 {-offset})">')
        y=top+24
        text(x,y,"Language and appearance","語言與外觀",18,weight=600)
        y+=48 if lang=="bilingual" else 36
        appearance=("深色" if theme=="dark" else "跟隨系統") if lang=="cantonese" else ("Dark / 深色" if theme=="dark" else "System / 跟隨系統") if lang=="bilingual" else "Dark" if theme=="dark" else "System"
        for en,zh,val in [("Interface language","介面語言","English + 廣東話" if lang=="bilingual" else "廣東話" if lang=="cantonese" else "English"),("Appearance","外觀",appearance)]:
            text(x,y,en,zh,14,weight=600)
            bx=x+cw*.45 if wide else x;by=y-14 if wide else y-14+label_height+8;bw=cw*.55-16 if wide else cw-16
            box(bx,by,bw,ch,"nav",8)
            text(bx+14,by+23,val,sz=14,weight=500)
            out.append(f'<path d="M{bx+bw-28} {by+ch/2-3}l5 5 5 -5" fill="none" stroke="{p["muted"]}" stroke-width="1.5"/>')
            if wide:text(x,y-14+label_height+8+12,"Shared across native windows","套用到原生視窗",12,"muted")
            y+=row_height
        box(x,y,cw-16,80,"selected",8)
        text(x+14,y+24,"Preview: monitoring is active","預覽：監察運作中",14)
        y+=104
        if surface=="setup":
            button(x,y,200,"Apply preferences","套用介面偏好",True)
            button(x+208,y,128,"Use saved","用已儲存值")
            y+=ch+32
            for en,zh in [("English style","英文風格"),("Cantonese style","廣東話風格")]:
                text(x,y,en,zh,14,weight=600);y+=label_height+8
                box(x,y,cw-72,4,"line",2);box(x,y,cw-72,4,"accent",2);box(x+cw-84,y-8,20,20,"accent",10)
                text(x+cw-44,y+6,"5",sz=14);y+=36
        out.append('</g></g>')
        scroll(w-12,top,view,content,offset)
        box(176,fy,w-176,h-fy,"panel");line(176,fy,w,fy)
        text(x,fy+22,"Unsaved interface changes","介面變更尚未儲存",12,"muted")
        by=fy+(54 if lang=="bilingual" else 32)
        if surface=="settings":
            button(x,by,116,"Apply","套用",True);button(x+124,by,128,"Use saved","用已儲存值");button(x+260,by,96,"Reset","重設",plain=True)
        else:
            button(x,by,88,"Back","上一步");button(x+96,by,164,"Save and close","儲存並關閉");button(w-112,by,88,"Next","下一步",True)
    elif surface=="workspace":
        nav([("Monitor","監察"),("History","歷史"),("Plugins","外掛"),("Settings","設定"),("Setup","初始設定")])
        x=200;cw=w-x-24
        text(x,40,"Network overview","網絡總覽",24,weight=600)
        sy=86 if lang=="bilingual" else 62;search(x,sy,cw)
        ay=sy+ch+28
        button(x,ay,132,"Pause display","暫停顯示")
        button(x+140,ay,156,"Restart samples","重新採樣")
        button(x+304,ay,204,"Floating monitor","浮動監察器")
        top=ay+ch+12;view=h-top-16
        content=320+ch
        offset=max(0,content-view) if state!="initial" else 0
        out.append(f'<defs><clipPath id="monitor"><rect x="{x}" y="{top}" width="{cw}" height="{view}"/></clipPath></defs><g clip-path="url(#monitor)"><g transform="translate(0 {-offset})">')
        y=top+20
        for i,(en,zh,val) in enumerate([("Download","下載","24.8 MB/s"),("Upload","上載","1.2 MB/s"),("Today","今日","8.46 GB")]):
            xx=x+i*cw/3
            text(xx,y,en,zh,12,"muted");text(xx,y+(58 if lang=="bilingual" else 42),val,sz=24,weight=600)
            if i:line(xx-14,y-12,xx-14,y+62)
        gy=y+84;gh=120
        box(x,gy,cw,gh,"panel",10)
        text(x+16,gy+25,"Traffic · last 60 seconds","流量 · 最近 60 秒",14,weight=600)
        for yy in (gh*.4,gh*.65,gh*.85):line(x+16,gy+yy,w-40,gy+yy)
        pts=[(x+16+i*(cw-32)/16,gy+gh*.78-([0,4,12,7,30,45,32,27,55,65,38,40,53,28,62,50,58][i])*gh/150) for i in range(17)]
        out.append('<polyline fill="none" stroke="'+p['accent']+'" stroke-width="2.5" points="'+' '.join(f'{a},{b}' for a,b in pts)+'"/>')
        ty=gy+gh+16
        for i,(en,zh) in enumerate([("Adapters","網絡介面"),("Sample history","採樣記錄"),("Details","詳細資料")]):
            active=i==({"samples":1,"details":2}.get(state,0))
            if active:box(x+i*166,ty,158,ch,"selected",8)
            text(x+i*166+12,ty+21,en,zh,14,"accent" if active else "muted")
        ty+=ch+12;box(x,ty,cw,52,"panel",8)
        en,zh=("12:00:01  ·  24.8 MB/s","12:00:01  ·  24.8 MB/s") if state=="samples" else ("Ethernet · connected · 1 Gbps","有線網絡 · 已連線 · 1 Gbps") if state=="details" else ("Ethernet","有線網絡")
        text(x+12,ty+20,en,zh,14,weight=500)
        out.append('</g></g>')
        content=ty+52-top+16
        scroll(w-12,top,view,content,offset)
    elif surface=="menu":
        box(0,0,w,h,"panel",10,"line");text(20,30,"Interface language","介面語言",18,weight=600)
        sy=72 if lang=="bilingual" else 48;search(16,sy,w-32)
        yy=sy+ch+44
        for en,zh,checked in [("English","英文",False),("Cantonese","廣東話",False),("English + Cantonese","英文 + 廣東話",True)]:
            if checked:box(8,yy-24,w-24,ch+4,"selected",6)
            text(20,yy,"✓" if checked else "",sz=16,color="accent")
            text(48,yy,en,zh,14);yy+=ch+12
        # All three options fit these reference viewports; no scrollbar is drawn.
        line(16,h-48,w-16,h-48);text(16,h-25,"Enter selects · Esc cancels","Enter 選擇 · Esc 取消",12,"muted")
    else:
        x=24;cw=w-48;text(x,38,"Regular expression","正則表達式",24,weight=600)
        sy=84 if lang=="bilingual" else 60;search(x,sy,cw)
        bottom=h-(114 if lang=="bilingual" else 80)
        top=sy+ch+24;view=bottom-top
        content=70+2*ch+(160 if w>=880 else 226)+16
        offset=70+2*ch+(126 if state=="results" else 0) if state in ("subject","results") else 0
        out.append(f'<defs><clipPath id="body"><rect x="0" y="{top}" width="{w-16}" height="{view}"/></clipPath></defs><g clip-path="url(#body)"><g transform="translate(0 {-offset})">')
        yy=top+22
        for i,(en,zh) in enumerate([("Test","測試"),("Replace","取代"),("Explain","解說"),("Library","資料庫")]):
            if i==0:box(x+i*116,yy-20,108,ch,"selected",6)
            text(x+i*116+12,yy,en,zh,14,"accent" if i==0 else "muted",500)
        yy+=ch+8;field(x,yy,cw-112,"Pattern","模式","Ethernet",True);button(w-124,yy+8,100,"Flags","選項")
        yy+=ch+40
        if w>=880:
            col=(cw-16)/2
            for xx,en,zh in [(x,"Test text","測試文字"),(x+col+16,"Matches","相符結果")]:
                box(xx,yy,col,160,"panel",8,"line");text(xx+12,yy+24,en,zh,14,weight=600)
                text(xx+12,yy+(76 if lang=="bilingual" else 56),"Ethernet",sz=14)
        else:
            box(x,yy,cw,110,"panel",8,"line");text(x+12,yy+24,"Test text","測試文字",14);text(x+12,yy+76,"Ethernet",sz=14)
            yy+=126;box(x,yy,cw,100,"panel",8,"line");text(x+12,yy+24,"Matches","相符結果",14);text(x+12,yy+76,"0..8  Ethernet",sz=14)
        out.append('</g></g>')
        scroll(w-12,top,view,content,offset)
        box(0,bottom,w,h-bottom,"panel");line(0,bottom,w,bottom)
        text(x,bottom+22,"1 match · bounded local execution","1 個結果 · 本機限時執行",12,"muted")
        button(w-300,bottom+(54 if lang=="bilingual" else 32),128,"Cancel","取消");button(w-164,bottom+(54 if lang=="bilingual" else 32),140,"Use pattern","採用模式",True)
    out.append('</svg>')
    return '\n'.join(out)

for surface in SIZES:
    for size in ("normal","minimum"):
        for language in ("english","cantonese","bilingual"):
            for theme in PALETTES:
                (ROOT/f"{surface}-{size}-{language}-{theme}.svg").write_text(render(surface,size,language,theme),encoding="utf-8")

for theme in PALETTES:
    for size in ("normal","minimum"):
        for language in ("english","cantonese","bilingual"):
            (ROOT/f"setup-{size}-{language}-{theme}-scrolled.svg").write_text(render("setup",size,language,theme,"scrolled"),encoding="utf-8")
        for state in ("samples","details"):
            (ROOT/f"workspace-{size}-bilingual-{theme}-{state}.svg").write_text(render("workspace",size,"bilingual",theme,state),encoding="utf-8")
    for state in ("subject","results"):
        (ROOT/f"regex-minimum-bilingual-{theme}-{state}.svg").write_text(render("regex","minimum","bilingual",theme,state),encoding="utf-8")
def components(theme):
    p=PALETTES[theme]
    parts=[f'<svg xmlns="http://www.w3.org/2000/svg" width="1080" height="780" viewBox="0 0 1080 780"><title>Native component anatomy, {theme}, design only</title><rect width="1080" height="780" fill="{p["bg"]}"/>']
    def t(x,y,s,size=14,color="text"):
        parts.append(f'<text x="{x}" y="{y}" font-family="Segoe UI,Microsoft JhengHei,sans-serif" font-size="{size}" fill="{p[color]}">{escape(s)}</text>')
    def r(x,y,w,h,fill="panel",stroke="line",radius=6,sw=1):
        parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{p.get(fill,fill)}" stroke="{p.get(stroke,stroke)}" stroke-width="{sw}"/>')
    t(32,42,"Native components / 原生元件",24);t(32,68,"Authored design proposal. Native semantics remain on the actual input HWND.",12,"muted")
    for i,(label,color) in enumerate([("Resting / 一般","line"),("Focused / 焦點","accent"),("Invalid / 無效","#C94450")]):
        x=32+i*344;r(x,104,320,64,stroke=color,sw=2 if i else 1);t(x+12,124,"Pattern / 模式",12,"muted");t(x+12,151,"Ethernet");t(x,194,label,12,"muted")
    t(32,242,"Action hierarchy / 操作層級",18)
    for x,label,fill,ink in [(32,"Apply / 套用","accent","ink"),(222,"Use saved / 用已儲存值","selected","text"),(480,"Cancel / 取消","panel","text")]:
        r(x,264,172 if x!=222 else 240,44,fill,radius=12);t(x+12,291,label,14,ink)
    t(700,291,"Reset / 重設",14,"accent");t(32,346,"Selection and range / 選擇與數值",18)
    r(32,370,20,20,"accent","accent",3);t(35,386,"✓",14,"ink");t(64,386,"Show taskbar monitor / 顯示工作列監察")
    r(490,366,44,26,"accent","accent",13);r(514,369,20,20,"ink","ink",10);t(548,385,"Enabled / 已啟用")
    r(32,426,360,4,"line","line",2);r(32,426,210,4,"accent","accent",2);r(232,418,20,20,"accent","accent",10);t(410,434,"60 s / 秒")
    t(32,484,"Scroll rail / 捲動軌",18);t(32,508,"Rest · hover · drag · focus. Reserved gutter, never text overlap.",12,"muted")
    for i,label in enumerate(["Rest","Hover","Drag","Focus"]):
        x=40+i*150;r(x,532,12,170,"nav","nav",6);r(x+(3 if i==0 else 2),554,6 if i==0 else 8,58,"accent" if i==2 else "muted","accent" if i==3 else "muted",4,2 if i==3 else 1);t(x+22,574,label,12)
    r(676,524,364,174,"panel","line",10);r(684,534,348,52,"selected","selected",6);t(700,556,"✓  English + Cantonese");t(724,577,"英文 + 廣東話",12,"muted");t(700,616,"Cantonese");t(700,638,"廣東話",12,"muted");t(700,676,"Checked result row / 已選結果",12,"muted")
    t(32,750,"High contrast uses system colors; DPI scales measured geometry. These are not runtime screenshots.",12,"muted")
    return '\n'.join(parts)+"</svg>"
for theme in PALETTES:
    (ROOT/f"components-{theme}.svg").write_text(components(theme),encoding="utf-8")
print("Generated 84 surface references and 2 component references. No runtime evidence produced.")
