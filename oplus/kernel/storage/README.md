# 目录结构说明
## common
存放所有平台通用模块代码
## mtk
存放MTK平台特有模块
## qcom
存放qcom平台特有模块代码
# 开发feature说明
1. 独立的feature模块放在对应目录下独立的子目录
2. 当前目录下的Kconfig、Makefile为in-tree编译方式存在（OKI），out-of-tree解耦后的编译用不到
3. 匹配不同版本的内核可以采用LINUX_VERSION_CODE、KERNEL_VERSION()去控制
# 编译说明
1. 子目录的编译采用out-of-tree方式编译，修改仓库oplus/kernel/build中相应的配置
2. 子目录的编译采用in-tree方式编译，修改当前目录的Kconfig
