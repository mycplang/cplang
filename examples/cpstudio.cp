// ═══════════════════════════════════════════════════════════
// CP Studio v0.8 — 调试器+终端面板 集成
// ═══════════════════════════════════════════════════════════

变量 CP关键字=["函数","变量","如果","否则","当","返回","打印","类型","导入","结构","枚举","类","真","假","nil","可信","可写","func","var","if","else","while","return","print","type","import","struct","enum","class","true","false","null"];
变量 CP类型=["整数","浮点","字符串","布尔","表","数组","集合","int","float","string","bool","table","array","set"];
变量 CP内置=["打印","长度","push","pop","读取文件","写入文件","文件存在","目录列表","procSystem","strlen","substr","table","表设","表取","表有","表长","字符串包含","字符串替换","执行命令"];

变量 所有符号=[]; 变量 标签页=[]; 变量 当前标签=0;
变量 输出内容=""; 变量 问题列表=[]; 变量 文件树列表=[]; 变量 当前目录=".";
变量 搜索文本=""; 变量 替换文本=""; 变量 显示搜索=假;
变量 侧栏宽=240; 变量 窗口宽=1200; 变量 窗口高=800;
变量 显示命令面板=假; 变量 命令搜索=""; 变量 命令列表=[];
变量 显示转到行=假; 变量 转到行号="1"; 变量 分屏=假;
变量 最近文件=[]; 变量 显示补全=假; 变量 补全候选=[]; 变量 补全搜索="";
变量 Git分支=""; 变量 Git变更=[]; 变量 显示GitDiff=假; 变量 GitDiff内容="";
变量 当前主题=0; 变量 显示欢迎=真; 变量 面包屑="";
变量 断点列表=[];

// ── 调试器状态 ──
变量 调试中=假; 变量 调试暂停=假; 变量 调试当前行=0;
变量 终端的fd=-1; 变量 终端内容="";

// ── 主题 ──
变量 主题名=["暗黑","午夜蓝","森林绿","暖棕","亮白","紫金","海洋","石墨"];

// ── 符号初始化 ──
函数 初始化符号(){
    所有符号=[];j=0;当(j<长度(CP关键字)){追加(所有符号,CP关键字[j]);j=j+1;}
    j=0;当(j<长度(CP类型)){追加(所有符号,CP类型[j]);j=j+1;}
    j=0;当(j<长度(CP内置)){追加(所有符号,CP内置[j]);j=j+1;}
}

// ── 标签页 ──
函数 新建标签(){t=table();表设(t,"name","未命名.cp");表设(t,"path","");表设(t,"content","");表设(t,"modified",假);追加(标签页,t);当前标签=长度(标签页)-1;显示欢迎=假;}
函数 当前内容(){如果(长度(标签页)==0){新建标签();}返回 表取(标签页[当前标签],"content");}
函数 设当前内容(v){表设(标签页[当前标签],"content",v);表设(标签页[当前标签],"modified",真);显示欢迎=假;}
函数 当前文件(){如果(长度(标签页)==0){返回"未命名";}p=表取(标签页[当前标签],"path");返回 p==""?表取(标签页[当前标签],"name"):p;}

// ── 文件操作 ──
函数 打开文件(path){
    如果(!文件存在(path)){输出内容="不存在: "+path;返回;}
    c=读取文件(path);显示欢迎=假;
    i=0;当(i<长度(标签页)){如果(表取(标签页[i],"path")==path){当前标签=i;面包屑=path;返回;}i=i+1;}
    t=table();表设(t,"name",path);表设(t,"path",path);表设(t,"content",c);表设(t,"modified",假);
    追加(标签页,t);当前标签=长度(标签页)-1;输出内容="已打开: "+path;面包屑=path;追加(最近文件,path);
}
函数 保存文件(){p=表取(标签页[当前标签],"path");如果(p==""){p=表取(标签页[当前标签],"name");}写入文件(p,表取(标签页[当前标签],"content"));表设(标签页[当前标签],"modified",假);输出内容="已保存: "+p;}
函数 运行文件(){
    保存文件();
    变量 cplang_home = 获取环境变量("CPLANG_HOME");
    如果 (cplang_home == "") { cplang_home = "C:\\cplang"; }
    变量 tmp_file = 获取环境变量("TEMP");
    如果 (tmp_file == "") { tmp_file = "."; }
    tmp_file = tmp_file + "\\_cp_run.cp";
    写入文件(tmp_file,表取(标签页[当前标签],"content"));
    raw=procSystem(cplang_home + "\\build\\cplang.exe -c \"" + tmp_file + "\" 2>&1");
    输出内容=raw;问题列表=[];
    如果(字符串包含(raw,"失败")||字符串包含(raw,"错误")){追加(问题列表,"编译错误");}
}

// ── 调试器 ──
函数 切换断点(line){
    found=假; i=0;
    当(i<长度(断点列表)){
        如果(断点列表[i]==line){断点列表[i]=0;found=真;break;}
        i=i+1;
    }
    如果(!found){追加(断点列表,line);设置断点(line);}否则{清除断点(line);}
}
函数 开始调试(){
    保存文件(); 设置断点(1);  // 在第一行暂停
    调试中=真; 调试暂停=假;
    运行文件();
}
函数 继续调试(){继续执行();调试暂停=假;}
函数 单步调试(){单步执行();调试暂停=假;}
函数 停止调试(){停止调试();调试中=假;调试暂停=假;}

// ── 终端 ──
函数 打开终端(){
    如果(终端的fd<0){
        终端的fd=终端打开();
        终端内容="终端已启动\n";
    }
}
函数 刷新终端(){
    如果(终端的fd>=0){
        data=终端读取(终端的fd);
        如果(data!=""){终端内容=终端内容+data;}
    }
}
函数 终端输入(cmd){
    如果(终端的fd>=0){
        终端写入(终端的fd,cmd+"\n");
        终端内容=终端内容+"$ "+cmd+"\n";
    }
}

// ── 图标/词法 ──
函数 图标(p){如果(字符串包含(p,".cp")){返回"🟣";}如果(字符串包含(p,".hpp")||字符串包含(p,".cpp")){返回"⚙️";}如果(字符串包含(p,".md")){返回"📖";}返回"📄";}
函数 是关键字(w){i=0;当(i<长度(CP关键字)){如果(CP关键字[i]==w){返回真;}i=i+1;}返回假;}
函数 是内置(w){i=0;当(i<长度(CP内置)){如果(CP内置[i]==w){返回真;}i=i+1;}返回假;}

// ── Git ──
函数 Git刷新(){
    Git分支=procSystem("git branch --show-current 2>/dev/null");
    如果(Git分支==""){Git分支="(无)";Git变更=[];返回;}
    raw=procSystem("git status --short 2>/dev/null");
    Git变更=[];line="";i=0;
    当(i<长度(raw)){如果(子串(raw,i,1)=="\n"){如果(line!=""){追加(Git变更,line);}line="";}否则{line=line+子串(raw,i,1);}i=i+1;}
}
函数 Git显示Diff(){GitDiff内容=procSystem("git diff HEAD 2>/dev/null");如果(GitDiff内容==""){GitDiff内容="无变更";}显示GitDiff=真;}

// ── 文件树 ──
函数 刷新树(){文件树列表=目录列表(当前目录);如果(文件树列表==nil){文件树列表=[];}}
函数 绘制树(){
    如果(igBegin("资源管理器",真)){
        如果(igButton("↻")){刷新树();Git刷新();}igSameLine();igText(当前目录+" "+Git分支);
        igSeparator();
        快入=["📂 项目","📦 包","📚 示例","🔀 Git"]; dirs=[".","/root/.cpkg/packages","examples","."];
        j=0;当(j<4){如果(igButton(快入[j])){如果(j<3){当前目录=dirs[j];刷新树();}否则{Git显示Diff();}}j=j+1;}
        igSeparator();
        i=0;当(i<长度(文件树列表)){name=文件树列表[i];full=当前目录+"/"+name;
            如果(是目录(full)){如果(igTreeNode("📁 "+name,0)){igTreePop();}}
            否则{如果(igButton(图标(name)+" "+name)){打开文件(full);}}
            i=i+1;
        }
        igEnd();
    }
}

// ── 标签栏+编辑器 ──
函数 绘制标签栏(){
    如果(长度(标签页)==0){返回;}
    如果(igBeginTabBar("tabs")){
        i=0;当(i<长度(标签页)){nm=表取(标签页[i],"name");如果(nm==""){i=i+1;continue;}
            mod=表取(标签页[i],"modified");title=图标(nm)+" "+nm;如果(mod){title="● "+title;}
            如果(igBeginTabItem(title,真)){当前标签=i;igEndTabItem();}
            i=i+1;
        }
        igEndTabBar();
    }
}
函数 绘制编辑器(isRight){
    c=当前内容();tl=1;j=0;当(j<长度(c)){如果(子串(c,j,1)=="\n"){tl=tl+1;}j=j+1;}
    pid="##editor";如果(isRight){pid="##editor2";}
    如果(igBegin(pid,真)){
        // 工具栏
        如果(igButton("📂")){打开文件("test.cp");}igSameLine();
        如果(igButton("💾")){保存文件();}igSameLine();
        如果(igButton("▶")){运行文件();}igSameLine();
        如果(igButton("🔍")){显示搜索=!显示搜索;}igSameLine();
        如果(igButton("⇋")){分屏=!分屏;}igSameLine();
        如果(igButton("🎨")){当前主题=当前主题+1;如果(当前主题>7){当前主题=0;}}igSameLine();
        // 调试按钮
        如果(igButton("🔴")){如果(!调试中){开始调试();}否则{停止调试();}}igSameLine();
        如果(igButton("⏭")){单步调试();}igSameLine();
        如果(igButton("▶▶")){继续调试();}igSameLine();
        igText("| "+图标(当前文件())+" "+tl+"行 | "+主题名[当前主题]);
        igSeparator();igText("▸ "+面包屑);igSeparator();
        
        如果(显示搜索){
            搜索文本=igInputText("查找:",搜索文本,256);igSameLine();
            替换文本=igInputText("替换:",替换文本,256);igSameLine();
            如果(igButton("替换")){设当前内容(字符串替换(c,搜索文本,替换文本));}
            igSeparator();
        }
        
        // 行号+代码
        igBeginChild("##gutter",50,-1,假);
        pushStyleColor(0,100,100,100,255);
        ln=1;当(ln<=tl){
            // 断点标记
            isBp=假;k=0;当(k<长度(断点列表)){如果(断点列表[k]==ln){isBp=真;break;}k=k+1;}
            如果(isBp){pushStyleColor(0,233,69,96,255);igText("●"+ln);popStyleColor(1);}
            否则{igText(ln);}
            如果(igIsItemClicked()){切换断点(ln);}
            ln=ln+1;
        }
        popStyleColor(1);
        igEndChild();igSameLine();
        
        igBeginChild("##code",-1,-1,假);
        nc=igInputTextMultiline("##source",c,50000);
        如果(nc!=c){设当前内容(nc);}
        igEndChild();
        igEnd();
    }
}

// ── 调试面板 ──
函数 绘制调试面板(){
    如果(igBegin("调试",真)){
        如果(调试中){
            如果(igButton("⏭ 单步(F10)")){单步调试();}igSameLine();
            如果(igButton("▶▶ 继续(F5)")){继续调试();}igSameLine();
            如果(igButton("⏹ 停止")){停止调试();}
            igSeparator();
            
            暂停状态=是否暂停();当前行=当前调试行();
            igText("状态: "+(暂停状态?"⏸ 暂停":"▶ 运行中")+" | 行: "+当前行);
            igText("调用栈: "+调用栈());
            
            igSeparator();
            igText("局部变量: "+局部变量());
            
            // 断点列表
            igSeparator(); igText("断点:");
            k=0;当(k<长度(断点列表)){
                如果(断点列表[k]>0){
                    igText("  ● 第"+断点列表[k]+"行");
                }
                k=k+1;
            }
        }否则{
            如果(igButton("🔴 开始调试")){开始调试();}
            igText("点击行号设置断点, 然后开始调试");
        }
        igEnd();
    }
}

// ── 终端面板 ──
函数 绘制终端(){
    如果(igBegin("终端",真)){
        如果(终端的fd<0){
            如果(igButton(">_ 打开终端")){打开终端();}
        }否则{
            刷新终端();
            igText(终端内容);
            igSeparator();
            cmd=igInputText("$","",256);
            如果(igButton("执行")){终端输入(cmd);}
        }
        igEnd();
    }
}

// ── 输出/问题面板 ──
函数 绘制输出(){
    如果(igBegin("输出",真)){
        如果(igButton("🗑")){输出内容="";问题列表=[];}
        err=字符串包含(输出内容,"失败")||字符串包含(输出内容,"错误");
        如果(err){pushStyleColor(0,242,71,71,255);}igText(输出内容);如果(err){popStyleColor(1);}
        igEnd();
    }
}
函数 绘制问题(){
    如果(igBegin("问题",真)){如果(igButton("🔄")){运行文件();}
        如果(长度(问题列表)==0){igText("✅ 无问题");}
        i=0;当(i<长度(问题列表)){pushStyleColor(0,242,71,71,255);igText("❌ "+问题列表[i]);popStyleColor(1);i=i+1;}
        igEnd();
    }
}

// ── 补全+命令面板+转到行 ──
函数 触发补全(){
    补全候选=[];补全搜索="";c=当前内容();j=长度(c)-1;
    当(j>=0){ch=子串(c,j,1);如果(ch==" "||ch=="\n"||ch=="("||ch==")"||ch=="{"||ch=="}"){break;}补全搜索=ch+补全搜索;j=j-1;}
    如果(补全搜索==""){返回;}
    k=0;当(k<长度(所有符号)){如果(字符串包含(所有符号[k],补全搜索)){追加(补全候选,所有符号[k]);如果(长度(补全候选)>=8){break;}}k=k+1;}
    显示补全=(长度(补全候选)>0);
}
函数 绘制补全(){
    如果(!显示补全||长度(补全候选)==0){显示补全=假;返回;}
    igSetNextWindowPos(300,300);igSetNextWindowSize(200,长度(补全候选)*25+30);
    如果(igBegin("补全",显示补全)){m=0;当(m<长度(补全候选)){sym=补全候选[m];icon="🔤";如果(是关键字(sym)){icon="🔑";}如果(是内置(sym)){icon="⚡";}
        如果(igButton(icon+" "+sym)){c=当前内容();newc=子串(c,0,长度(c)-长度(补全搜索))+sym;设当前内容(newc);显示补全=假;}m=m+1;}
        igEnd();
    }
}
函数 初始化命令(){命令列表=["📂 新建","📂 打开","💾 保存","▶ 运行","🔍 查找","⇋ 分屏","🔴 调试",">_ 终端","🔢 转到行","💡 补全","🎨 换主题","❓ 关于"];}
函数 绘制命令面板(){
    如果(!显示命令面板){返回;}
    igSetNextWindowPos(窗口宽/2-200,窗口高/2-150);igSetNextWindowSize(400,300);
    如果(igBegin("命令",显示命令面板)){命令搜索=igInputText(">",命令搜索,256);igSeparator();
        i=0;当(i<长度(命令列表)){cmd=命令列表[i];
            如果(命令搜索==""||字符串包含(cmd,命令搜索)){如果(igButton(cmd)){
                如果(字符串包含(cmd,"新建")){新建标签();}如果(字符串包含(cmd,"打开")){打开文件("test.cp");}
                如果(字符串包含(cmd,"保存")){保存文件();}如果(字符串包含(cmd,"运行")){运行文件();}
                如果(字符串包含(cmd,"查找")){显示搜索=真;}如果(字符串包含(cmd,"分屏")){分屏=!分屏;}
                如果(字符串包含(cmd,"调试")){开始调试();}如果(字符串包含(cmd,"终端")){打开终端();}
                如果(字符串包含(cmd,"转到行")){显示转到行=真;}如果(字符串包含(cmd,"补全")){触发补全();}
                如果(字符串包含(cmd,"主题")){当前主题=当前主题+1;如果(当前主题>7){当前主题=0;}}
                显示命令面板=假;命令搜索="";
            }}i=i+1;}
        igEnd();
    }
}
函数 绘制转到行(){
    如果(!显示转到行){返回;}
    igSetNextWindowPos(窗口宽/2-100,窗口高/2-30);igSetNextWindowSize(200,60);
    如果(igBegin("转到行",显示转到行)){转到行号=igInputText("##goto",转到行号,10);igSameLine();如果(igButton("✓")){显示转到行=假;}igEnd();}
}
函数 绘制状态栏(){
    c=当前内容();ln=1;col=1;j=0;当(j<长度(c)){如果(子串(c,j,1)=="\n"){ln=ln+1;col=1;}否则{col=col+1;}j=j+1;}
    word="";wj=j-1;当(wj>=0){ch=子串(c,wj,1);如果(ch==" "||ch=="\n"||ch=="("||ch==")"||ch=="{"||ch=="}"){break;}word=ch+word;wj=wj-1;}
    token="abc";如果(是关键字(word)){token="🔑";}如果(是内置(word)){token="⚡";}
    dbgInfo="";如果(调试中){dbgInfo=" | 🔴调试中";}如果(调试暂停){dbgInfo=" | ⏸ 暂停";}
    如果(igBegin("##status",真)){
        igText("Ln "+ln+" Col "+col+" | "+token+dbgInfo+" | "+Git分支+" | "+主题名[当前主题]+" | v0.8");
        igEnd();
    }
}

// ── 菜单 ──
函数 绘制菜单(){
    如果(igBeginMenuBar()){
        如果(igBeginMenu("文件",真)){如果(igMenuItem("新建","Ctrl+N",假,真)){新建标签();}如果(igMenuItem("打开","Ctrl+O",假,真)){打开文件("test.cp");}如果(igMenuItem("保存","Ctrl+S",假,真)){保存文件();}igEndMenu();}
        如果(igBeginMenu("编辑",真)){如果(igMenuItem("查找","Ctrl+F",假,真)){显示搜索=真;}如果(igMenuItem("转到行","Ctrl+G",假,真)){显示转到行=真;}igEndMenu();}
        如果(igBeginMenu("运行",真)){如果(igMenuItem("运行","F5",假,真)){运行文件();}igSeparator();如果(igMenuItem("开始调试","",假,真)){开始调试();}如果(igMenuItem("单步","F10",假,真)){单步调试();}igEndMenu();}
        如果(igBeginMenu("视图",真)){如果(igMenuItem("命令面板","Ctrl+Shift+P",假,真)){显示命令面板=真;初始化命令();}如果(igMenuItem("切换侧栏","",假,真)){侧栏宽=侧栏宽>0?0:240;}igEndMenu();}
        igEndMenuBar();
    }
}

// ── 快捷键 ──
函数 快捷键(){
    如果(igIsKeyDown(341)&&igIsKeyDown(340)&&igIsKeyPressed(80)){显示命令面板=真;初始化命令();}
    如果(igIsKeyDown(341)&&igIsKeyPressed(83)){保存文件();}
    如果(igIsKeyDown(341)&&igIsKeyPressed(70)){显示搜索=真;}
    如果(igIsKeyDown(341)&&igIsKeyPressed(78)){新建标签();}
    如果(igIsKeyDown(341)&&igIsKeyPressed(71)){显示转到行=真;}
    如果(igIsKeyDown(341)&&igIsKeyPressed(32)){触发补全();}
    如果(igIsKeyPressed(294)){运行文件();}
    如果(igIsKeyPressed(299)){单步调试();}  // F10
    如果(igIsKeyPressed(293)){// F9 切换断点
        c=当前内容();ln=1;j=0;当(j<长度(c)){如果(子串(c,j,1)=="\n"){ln=ln+1;}j=j+1;}
        切换断点(ln);
    }
}

// ── 欢迎页 ──
函数 绘制欢迎页(){
    igSetNextWindowPos(窗口宽/2-250,窗口高/2-200);igSetNextWindowSize(500,400);
    如果(igBegin("欢迎",0)){
        igText("🟣 CP Studio v0.8");igSeparator();
        igText("快捷键: Ctrl+N 新建 | Ctrl+S 保存 | F5 运行");
        igText("        F9 断点 | F10 单步 | Ctrl+Shift+P 命令");
        igText("        Ctrl+F 查找 | Ctrl+G 转到行");
        igSeparator();igText("最近:");
        i=0;当(i<长度(最近文件)&&i<5){rf=最近文件[长度(最近文件)-1-i];如果(igButton(图标(rf)+" "+rf)){打开文件(rf);显示欢迎=假;}i=i+1;}
        如果(igButton("新建文件")){新建标签();}igSameLine();如果(igButton("打开项目")){打开文件("test.cp");}
        igEnd();
    }
}

// ── 主循环 ──
函数 主循环(){
    初始化符号();Git刷新();
    打印("CP Studio v0.8 — 调试器+终端");
    刷新树();初始化窗口(窗口宽,窗口高,"CP Studio v0.8");igInit();设置目标帧率(60);
    当(!窗口应关闭()){
        快捷键();刷新终端();
        开始绘制();清空背景(30,30,30,255);igBeginDraw();
        igSetNextWindowPos(0,20);igSetNextWindowSize(窗口宽,窗口高-20);
        如果(igBegin("CP Studio",0)){
            绘制菜单();
            如果(侧栏宽>0){igBeginChild("sidebar",侧栏宽,-1,真);绘制树();igEndChild();igSameLine();}
            igBeginChild("main",-1,-1,真);
            如果(分屏){
                igBeginChild("sp1",-1,窗口高/2-140,真);绘制编辑器(假);igEndChild();
                igBeginChild("sp2",-1,-1,真);绘制编辑器(真);igEndChild();
            }否则 如果(显示欢迎&&长度(标签页)==0){
                绘制欢迎页();
            }否则{
                igBeginChild("editor_area",-1,-200,真);绘制编辑器(假);igEndChild();
                igBeginChild("bottom",-1,-1,真);
                如果(igBeginTabBar("btabs")){
                    如果(igBeginTabItem("输出",真)){绘制输出();igEndTabItem();}
                    如果(igBeginTabItem("问题",真)){绘制问题();igEndTabItem();}
                    如果(igBeginTabItem("调试",真)){绘制调试面板();igEndTabItem();}
                    如果(igBeginTabItem("终端",真)){绘制终端();igEndTabItem();}
                    igEndTabBar();
                }
                igEndChild();
            }
            igEndChild();igEnd();
        }
        绘制状态栏();绘制命令面板();绘制转到行();绘制补全();
        igEndDraw();结束绘制();
    }
    igShutdown();关闭窗口();
}
主循环();
