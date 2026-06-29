// CP语言 标准库汇总头文件
#pragma once

// 防止 Windows.h min/max 宏污染
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef min
#undef max

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <regex>
#include <filesystem>
#include <random>

// StdLib 注册类声明
#include "stdlib/stdlib_fwd.hpp"

// 各模块函数声明
#include "stdlib/stdlib_math.hpp"
#include "stdlib/stdlib_string.hpp"
#include "stdlib/stdlib_array.hpp"
#include "stdlib/stdlib_table.hpp"
#include "stdlib/stdlib_io.hpp"
#include "stdlib/stdlib_file.hpp"
#include "stdlib/stdlib_time_system.hpp"
#include "stdlib/stdlib_types_net.hpp"
#include "stdlib/stdlib_containers.hpp"
#include "stdlib/stdlib_algo_bitwise.hpp"
#include "stdlib/stdlib_regex_encoding_crypto.hpp"
#include "stdlib/stdlib_variant_utils.hpp"
#include "stdlib/stdlib_tcp.hpp"
#include "stdlib/stdlib_image.hpp"
#include "stdlib/stdlib_compress.hpp"
#include "stdlib/stdlib_argparse.hpp"
#include "stdlib/stdlib_config.hpp"
#include "stdlib/stdlib_httpserver.hpp"
#include "stdlib/stdlib_audio.hpp"
#include "stdlib/stdlib_markdown.hpp"
#include "stdlib/stdlib_template.hpp"
#include "stdlib/stdlib_websocket.hpp"
#include "stdlib/stdlib_pdf.hpp"
#include "stdlib/stdlib_kvdb.hpp"
#include "stdlib/stdlib_testing.hpp"
#include "stdlib/stdlib_logging.hpp"
#include "stdlib/stdlib_lang.hpp"
#include "stdlib/stdlib_binary.hpp"

// 游戏引擎模块 (v0.4.0 — 开箱即用)
#include "stdlib/stdlib_game_net.hpp"
#include "stdlib/stdlib_game_battle.hpp"
#include "stdlib/stdlib_game_map.hpp"
#include "stdlib/stdlib_game_item.hpp"
#include "stdlib/stdlib_game_role.hpp"
// #include "stdlib/stdlib_game_sprite.hpp"  -- WAS disabled
#include "stdlib/stdlib_game_db.hpp"
#include "stdlib/stdlib_game_config.hpp"
#include "stdlib/stdlib_web.hpp"