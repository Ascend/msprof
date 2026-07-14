# docs/zh agent读取导航

用途：agent快速路由到 `docs/zh` 下最合适的文档；当需要在线查看仓库文档原文时，也能按规则拼接文件页 URL 或 raw URL。

## 路径规则

### 本地仓库路径拼接

- 中文文档根目录：`docs/zh/`
- 文档已按主题分目录存放，优先使用完整相对路径，例如 `docs/zh/user_guide/profile_data_file_references.md`
- 如果只给出文档名且未标注子目录，需要先按目录分类补全，不要默认拼接成 `docs/zh/<filename>`
- 如果输入已经是 `docs/zh/...` 相对路径，则直接使用，不要重复拼接成 `docs/zh/docs/zh/...`
- 常见子目录映射：
  - `getting_started/<filename>` -> `docs/zh/getting_started/<filename>`
  - `user_guide/<filename>` -> `docs/zh/user_guide/<filename>`
  - `legal/<filename>` -> `docs/zh/legal/<filename>`
  - `development_guide/<filename>` -> `docs/zh/development_guide/<filename>`
  - `figures/<filename>` -> `docs/zh/figures/<filename>`
- `development_guide/` 和 `figures/` 默认不要主动读取，除非用户明确要求查看设计文档或图片

## docs/zh 结构

```text
docs/zh/
├── overview.md                                      // 工具是什么、能力范围、简介
├── dir_structure.md                                 // 仓库目录、模块位置、源码在哪
├── getting_started/
│   ├── quick_start.md                               // 快速跑通采集、解析、导出、初步分析
│   └── msprof_install_guide.md                      // 安装、编译、run包、依赖、卸载
├── user_guide/
│   ├── msprof_parsing_instruct.md                   // `msprof --export/--parse/--query/--analyze`等命令使用说明
│   ├── extended_functions.md                        // `msprof.py`、设备信息、分片、扩展功能
│   ├── profile_data_file_references.md              // 交付件如`.json`、`.csv` 文件和字段含义
│   └── profile_data_file_references_db.md           // DB格式性能数据文件说明
├── legal/
│   ├── security_statement.md                        // 权限、安全、漏洞、root、umask
│   ├── disclaimer.md                                // 免责声明
│   └── license_notice.md                            // 许可证声明
├── development_guide/                                          // 设计文档，默认不要主动读取
└── figures/                                         // 配图，默认不要主动读取
```
