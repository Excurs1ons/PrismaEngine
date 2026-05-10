#pragma once

namespace Prisma::UI {

    // --- Window Titles ---
    /// 主编辑器停靠空间窗口标题
    inline constexpr char const* WINDOW_DOCKSPACE = "Prisma Editor DockSpace";
    /// 场景视口窗口标题
    inline constexpr char const* WINDOW_VIEWPORT = "Viewport";
    /// 场景层级窗口标题
    inline constexpr char const* WINDOW_HIERARCHY = "Scene Hierarchy";
    /// 属性面板窗口标题
    inline constexpr char const* WINDOW_PROPERTIES = "Properties";
    /// 资源浏览器窗口标题
    inline constexpr char const* WINDOW_CONTENT_BROWSER = "Content Browser";
    /// 项目设置窗口标题
    inline constexpr char const* WINDOW_PROJECT_SETTINGS = "Project Settings";

    // --- Popup IDs ---
    /// 打开项目对话框 ID
    inline constexpr char const* POPUP_OPEN_PROJECT = "OpenProjectPopup";
    /// 新建项目对话框 ID
    inline constexpr char const* POPUP_NEW_PROJECT = "NewProjectPopup";
    /// 打开场景对话框 ID
    inline constexpr char const* POPUP_OPEN_SCENE = "OpenScenePopup";
    /// 项目另存为对话框 ID
    inline constexpr char const* POPUP_SAVE_PROJECT_AS = "SaveProjectAsPopup";
    /// 场景另存为对话框 ID
    inline constexpr char const* POPUP_SAVE_SCENE_AS = "SaveSceneAsPopup";
    /// 添加组件菜单 ID
    inline constexpr char const* POPUP_ADD_COMPONENT = "AddComponentPopup";

    // --- Menu Labels (Chinese) ---
    /// 文件菜单栏标题
    inline constexpr char const* MENU_FILE = "文件";
    /// 项目子菜单标题
    inline constexpr char const* MENU_PROJECT = "项目";
    /// 场景子菜单标题
    inline constexpr char const* MENU_SCENE = "场景";
    /// 编辑菜单栏标题
    inline constexpr char const* MENU_EDIT = "编辑";
    /// 视图菜单栏标题
    inline constexpr char const* MENU_VIEW = "视图";
    
    // --- Menu Items ---
    /// 打开项目菜单项
    inline constexpr char const* ITEM_OPEN_PROJECT = "打开项目...";
    /// 新建项目菜单项
    inline constexpr char const* ITEM_NEW_PROJECT = "新建项目...";
    /// 保存项目菜单项
    inline constexpr char const* ITEM_SAVE_PROJECT = "保存项目";
    /// 项目另存为菜单项
    inline constexpr char const* ITEM_SAVE_PROJECT_AS = "项目另存为...";
    /// 新建场景菜单项
    inline constexpr char const* ITEM_NEW_SCENE = "新建场景";
    /// 打开场景菜单项
    inline constexpr char const* ITEM_OPEN_SCENE = "打开场景...";
    /// 保存场景菜单项
    inline constexpr char const* ITEM_SAVE_SCENE = "保存场景";
    /// 场景另存为菜单项
    inline constexpr char const* ITEM_SAVE_SCENE_AS = "场景另存为...";
    /// 退出程序菜单项
    inline constexpr char const* ITEM_EXIT = "退出";
    /// 项目设置菜单项
    inline constexpr char const* ITEM_PROJECT_SETTINGS = "项目设置";
    /// 显示 ImGui Demo 窗口菜单项
    inline constexpr char const* ITEM_IMGUI_DEMO = "ImGui 演示窗口";
    /// 保存设置按钮文本
    inline constexpr char const* ITEM_SAVE_SETTINGS = "保存设置";

} // namespace Prisma::UI
