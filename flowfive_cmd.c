#include <vnet/plugin/plugin.h>
#include <flowfive/flowfive.h>


int
flowfive_enable_disable(u32 sw_if_index, int enable) {
    if (pool_is_free_index(flowfive_main.vnet_main->interface_main.sw_interfaces, sw_if_index)) {
        return VNET_API_ERROR_INVALID_SW_IF_INDEX;
    }

    vnet_feature_enable_disable ("ip4-unicast", "flowfive", sw_if_index, enable, 0, 0);

    return 0;
}

static clib_error_t *
set_flowfive_interface_command_fn(vlib_main_t *vm, unformat_input_t *input, vlib_cli_command_t *cmd) {
    u32 sw_if_index = ~0;
    int enable_disable = -1; // 默认情况下为 -1，表示未指定

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "disable")) {
            enable_disable = 0; // 禁用
        } else if (unformat(input, "enable")) {
            enable_disable = 1; // 启用
        } else if (unformat(input, "%U", unformat_vnet_sw_interface, flowfive_main.vnet_main, &sw_if_index)) {
            ;
        } else {
            if (unformat(input, "?")) {
                vlib_cli_output(vm, "flowfive <interface-name> [enable|disable]");
                return 0;
            }
            break;
        }
    }

    if (sw_if_index == ~0) {
        return clib_error_return(0, "not found found interface");
    }

    // 检查是否指定了启用或禁用选项
    if (enable_disable == -1) {
        return clib_error_return(0, "Please specify 'enable' or 'disable' option...");
    }

    // 增加调试输出
    vlib_cli_output(vm, "Setting FlowFive for interface %U to %s",
                    format_vnet_sw_if_index_name, flowfive_main.vnet_main, sw_if_index,
                    enable_disable ? "enable" : "disable");

    // 更新接口的启用状态
    interface_t *iface = NULL;
    int found = 0; // 增加 found 标志

    vec_foreach(iface, flowfive_main.interfaces) {
        if (iface->sw_if_index == sw_if_index) {
            iface->enabled = enable_disable;
            found = 1; // 找到匹配的接口
            vlib_cli_output(vm, "Updated interface %U to %s\n",
                            format_vnet_sw_if_index_name, flowfive_main.vnet_main, iface->sw_if_index,
                            enable_disable ? "enabled" : "disabled");
            break;
        }
    }

    // 如果未找到接口，则添加新接口
    if (!found) {
        interface_t new_iface;
        new_iface.sw_if_index = sw_if_index;
        new_iface.enabled = enable_disable;
        vec_add1(flowfive_main.interfaces, new_iface);
        vlib_cli_output(vm, "Added new interface %U as %s",
                        format_vnet_sw_if_index_name, flowfive_main.vnet_main, sw_if_index,
                        enable_disable ? "enabled" : "disabled");
    }

    flowfive_enable_disable(sw_if_index, enable_disable);

    // 增加调试输出
    vlib_cli_output(vm, "Current interfaces:");
    vec_foreach(iface, flowfive_main.interfaces) {
        vlib_cli_output(vm, "  Interface %U: %s",
                        format_vnet_sw_if_index_name, flowfive_main.vnet_main, iface->sw_if_index,
                        iface->enabled ? "enabled" : "disabled");
    }

    return 0;
}

static clib_error_t *
show_flowfive_interface_command_fn(vlib_main_t *vm, unformat_input_t *input, vlib_cli_command_t *cmd) {
    flowfive_main_t *fm = &flowfive_main;
    interface_t *iface;
    int found = 0;

    vlib_cli_output(vm, "Enabled FlowFive on the following interfaces:\n");

    vec_foreach(iface, fm->interfaces) {
        if (iface->enabled) {
            vlib_cli_output(vm, "  Interface: %U",
                            format_vnet_sw_if_index_name, fm->vnet_main, iface->sw_if_index);
            found = 1;
        }
    }

    if (!found) {
        vlib_cli_output(vm, "  No interfaces with FlowFive enabled found.");
    }

    return 0;
}


static clib_error_t *
show_flowfive_rsyslog_command_fn(vlib_main_t *vm, unformat_input_t *input, vlib_cli_command_t *cmd) {
    flowfive_main_t *ffm = &flowfive_main;

    vlib_cli_output(vm, "FlowFive Rsyslog Configuration:");
    vlib_cli_output(vm, "  host: %s", ffm->rsyslog.host);
    vlib_cli_output(vm, "  port: %d", ffm->rsyslog.port);
    vlib_cli_output(vm, "  facility: %d", ffm->rsyslog.facility);
    vlib_cli_output(vm, "  severity: %d", ffm->rsyslog.severity);
    vlib_cli_output(vm, "  log-type: %d", ffm->log_type);
    vlib_cli_output(vm, "  cache-size: %d", ffm->cache_size);
    vlib_cli_output(vm, "  cache-size-cursize: %d", ffm->queue->cursize);

    return 0;
}

static inline clib_error_t * validate_integer_range(i32 value, i32 min, i32 max, char *name) {
    if (value >= min && value <= max) {
        return NULL;
    } else {
        return clib_error_return(0, "error %s value, %d ~ %d", name, min, max);
    }
}

// 设置 rsyslog 配置命令函数
static clib_error_t *
set_flowfive_rsyslog_command_fn(vlib_main_t *vm, unformat_input_t *input, vlib_cli_command_t *cmd) {
    flowfive_main_t *ffm = &flowfive_main;
    int found = 0;
    u8 *host = NULL;
    i32 port;
    i32 facility;
    i32 severity;
    i32 type;
    struct sockaddr_in sa;
    clib_error_t *error = NULL;


    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "host %s", &host)) {
            if (inet_pton(AF_INET, (char *)host, &(sa.sin_addr)) == 1) {
                ffm->rsyslog.host = vec_dup(host);
                vec_free(host);
                found = 1;
            } else {
                vec_free(host);
                return clib_error_return(0, "error host IP");
            }
        } else if (unformat(input, "port %d", &port)) {
            error = validate_integer_range(port, 0, 65535, "port");
            if (error) {
                return error;
            }
            ffm->rsyslog.port = port;
            found = 1;
        } else if (unformat(input, "facility %d", &facility)) {
            error = validate_integer_range(facility, 0, 23, "facility");
            if (error) {
                return error;
            }
            ffm->rsyslog.facility = facility;
            found = 1;
        } else if (unformat(input, "severity %d", &severity)) {
            error = validate_integer_range(severity, 0, 7, "severity");
            if (error) {
                return error;
            }
            ffm->rsyslog.severity = severity;
            found = 1;
        } else if (unformat(input, "log-type %d", &type)) {
            error = validate_integer_range(type, 1, 2, "log-type");
            if (error) {
                return error;
            }
            ffm->log_type = type;
            found = 1;
        } else {
            break;
        }
    }

    if (!found) {
        return clib_error_return(0, "Please specify [host|port|facility|severity|log-type]");
    }

    // 根据新的 facility 和 severity 更新 rsyslog id
    ffm->rsyslog.id = (ffm->rsyslog.facility << 3) + ffm->rsyslog.severity;

    // 如果端口或主机改变，则更新 servaddr 结构
    ffm->servaddr.sin_family = AF_INET;
    ffm->servaddr.sin_port = htons(ffm->rsyslog.port);
    ffm->servaddr.sin_addr.s_addr = inet_addr(ffm->rsyslog.host);

    ffm->servaddr_ptr = (struct sockaddr *)&ffm->servaddr;
    ffm->servaddr_size = sizeof(ffm->servaddr);

    vlib_cli_output(vm, "Rsyslog ok");

    return 0;
}


// Show FlowFive Rsyslog Configuration command
VLIB_CLI_COMMAND (show_flowfive_rsyslog_command, static) = {
    .path = "show flowfive rsyslog",
    .short_help = "Show Rsyslog Configuration",
    .function = show_flowfive_rsyslog_command_fn,
};


// 查看配置命令
VLIB_CLI_COMMAND (show_flowfive_interface_command, static) = {
    .path = "show flowfive interface",
    .short_help = "Show all interfaces with FlowFive enabled",
    .function = show_flowfive_interface_command_fn,
};


// 设置配置命令
VLIB_CLI_COMMAND (set_flowfive_interface_command, static) = {
    .path = "set flowfive interface",
    .short_help = "set flowfive interface <interface-name> [enable|disable]",
    .function = set_flowfive_interface_command_fn,
};


// 设置配置命令
VLIB_CLI_COMMAND (set_flowfive_rsyslog_command, static) = {
    .path = "set flowfive rsyslog",
    .short_help = "set flowfive rsyslog [host|port|facility|severity|log-type] [value]",
    .function = set_flowfive_rsyslog_command_fn,
};
