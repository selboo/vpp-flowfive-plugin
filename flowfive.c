#include <vnet/plugin/plugin.h>
#include <flowfive/flowfive.h>

// 注册插件
VLIB_PLUGIN_REGISTER () = {
    .version = FLOWFIVE_PLUGIN_BUILD_VER,
    .description = FLOWFIVE_PLUGIN_DESCRIPTION,
};

// 注册错误
static clib_error_t *
flowfive_init (vlib_main_t *vm) {
    flowfive_main.vnet_main = vnet_get_main();
    return 0;
}

// 启动初始化操作
VLIB_INIT_FUNCTION (flowfive_init);

VNET_FEATURE_INIT (flowfive, static) = {
    .arc_name = "ip4-unicast",
    .node_name = "flowfive",
    .runs_before = VNET_FEATURES ("ip4-lookup"),
};
