"""
Prisma Engine Blender Addon
在Blender内实现与Prisma Engine的实时跨进程编辑
参考Unity Blender插件架构设计
"""

bl_info = {
    "name": "Prisma Engine Connector",
    "author": "Prisma Engine Team",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Prisma",
    "description": "实时连接Prisma Engine进行跨进程场景编辑",
    "category": "Development",
}

import bpy
import bmesh
import json
import socket
import threading
import queue
import time
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict
from enum import Enum
import struct
import pickle

# IPC通信协议
class MessageType(Enum):
    SCENE_UPDATE = 1
    OBJECT_ADD = 2
    OBJECT_REMOVE = 3
    OBJECT_TRANSFORM = 4
    MATERIAL_UPDATE = 5
    CAMERA_UPDATE = 6
    LIGHT_UPDATE = 7
    SYNC_REQUEST = 8
    SYNC_RESPONSE = 9
    HEARTBEAT = 10

@dataclass
class Vector3:
    x: float
    y: float
    z: float

@dataclass
class Quaternion:
    x: float
    y: float
    z: float
    w: float

@dataclass
class Transform:
    position: Vector3
    rotation: Quaternion
    scale: Vector3

@dataclass
class MeshData:
    vertices: List[float]
    normals: List[float]
    uvs: List[List[float]]
    triangles: List[int]
    name: str

@dataclass
class MaterialData:
    name: str
    diffuse_color: List[float]
    specular_color: List[float]
    roughness: float
    metallic: float
    textures: Dict[str, str]

@dataclass
class ObjectData:
    id: str
    name: str
    transform: Transform
    mesh_data: Optional[MeshData]
    material_data: Optional[MaterialData]
    type: str  # MESH, CAMERA, LIGHT

@dataclass
class SceneData:
    objects: List[ObjectData]
    cameras: List[ObjectData]
    lights: List[ObjectData]
    metadata: Dict[str, Any]

class PrismaIPCServer:
    """IPC服务器，负责与Prisma Engine通信"""
    
    def __init__(self, host='127.0.0.1', port=12345):
        self.host = host
        self.port = port
        self.socket = None
        self.connection = None
        self.running = False
        self.message_queue = queue.Queue()
        self.thread = None
        
    def start(self):
        """启动IPC服务器"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        self.socket.listen(1)
        
        self.running = True
        self.thread = threading.Thread(target=self._accept_connections)
        self.thread.daemon = True
        self.thread.start()
        
        print(f"Prisma IPC Server started on {self.host}:{self.port}")
        
    def stop(self):
        """停止IPC服务器"""
        self.running = False
        if self.connection:
            self.connection.close()
        if self.socket:
            self.socket.close()
            
    def _accept_connections(self):
        """接受客户端连接"""
        while self.running:
            try:
                conn, addr = self.socket.accept()
                print(f"Prisma Engine connected from {addr}")
                self.connection = conn
                self._handle_client(conn)
            except Exception as e:
                if self.running:
                    print(f"Connection error: {e}")
                    
    def _handle_client(self, conn):
        """处理客户端通信"""
        while self.running:
            try:
                # 接收消息头
                header = conn.recv(8)
                if not header:
                    break
                    
                msg_type, msg_size = struct.unpack('!II', header)
                
                # 接收消息体
                data = b''
                while len(data) < msg_size:
                    chunk = conn.recv(min(4096, msg_size - len(data)))
                    if not chunk:
                        break
                    data += chunk
                    
                if len(data) == msg_size:
                    self._process_message(msg_type, data)
                    
            except Exception as e:
                print(f"Error handling client: {e}")
                break
                
    def _process_message(self, msg_type: int, data: bytes):
        """处理接收到的消息"""
        try:
            message = pickle.loads(data)
            self.message_queue.put((msg_type, message))
        except Exception as e:
            print(f"Error processing message: {e}")
            
    def send_message(self, msg_type: MessageType, data: Any):
        """发送消息到Prisma Engine"""
        if not self.connection:
            return
            
        try:
            serialized = pickle.dumps(data)
            header = struct.pack('!II', msg_type.value, len(serialized))
            self.connection.sendall(header + serialized)
        except Exception as e:
            print(f"Error sending message: {e}")

class BlenderSceneExporter:
    """Blender场景导出器"""
    
    def __init__(self):
        self.object_cache = {}
        
    def export_scene(self) -> SceneData:
        """导出当前Blender场景"""
        scene = bpy.context.scene
        
        objects = []
        cameras = []
        lights = []
        
        # 导出所有对象
        for obj in scene.objects:
            object_data = self._export_object(obj)
            if object_data:
                if obj.type == 'CAMERA':
                    cameras.append(object_data)
                elif obj.type == 'LIGHT':
                    lights.append(object_data)
                else:
                    objects.append(object_data)
                    
        return SceneData(
            objects=objects,
            cameras=cameras,
            lights=lights,
            metadata={
                'scene_name': scene.name,
                'frame_current': scene.frame_current,
                'frame_end': scene.frame_end,
                'frame_start': scene.frame_start,
            }
        )
        
    def _export_object(self, obj) -> Optional[ObjectData]:
        """导出单个对象"""
        try:
            # 生成唯一ID
            obj_id = f"{obj.name}_{obj.pass_index}"
            
            # 导出变换
            transform = self._export_transform(obj)
            
            # 导出网格数据
            mesh_data = None
            if obj.type == 'MESH' and obj.data:
                mesh_data = self._export_mesh(obj)
                
            # 导出材质数据
            material_data = None
            if obj.data and hasattr(obj.data, 'materials'):
                material_data = self._export_material(obj)
                
            return ObjectData(
                id=obj_id,
                name=obj.name,
                transform=transform,
                mesh_data=mesh_data,
                material_data=material_data,
                type=obj.type
            )
        except Exception as e:
            print(f"Error exporting object {obj.name}: {e}")
            return None
            
    def _export_transform(self, obj) -> Transform:
        """导出变换数据"""
        loc = obj.location
        rot = obj.rotation_quaternion if obj.rotation_mode == 'QUATERNION' else obj.rotation_euler.to_quaternion()
        scale = obj.scale
        
        return Transform(
            position=Vector3(loc.x, loc.y, loc.z),
            rotation=Quaternion(rot.x, rot.y, rot.z, rot.w),
            scale=Vector3(scale.x, scale.y, scale.z)
        )
        
    def _export_mesh(self, obj) -> MeshData:
        """导出网格数据"""
        mesh = obj.data
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bm.transform(obj.matrix_world)
        
        vertices = []
        normals = []
        uvs = []
        triangles = []
        
        # 导出顶点和法线
        for vert in bm.verts:
            vertices.extend([vert.co.x, vert.co.y, vert.co.z])
            normals.extend([vert.normal.x, vert.normal.y, vert.normal.z])
            
        # 导出UV
        uv_layers = []
        if bm.loops.layers.uv:
            uv_layer = bm.loops.layers.uv.active
            uv_data = []
            for face in bm.faces:
                for loop in face.loops:
                    uv = loop[uv_layer].uv
                    uv_data.extend([uv.x, uv.y])
            uv_layers.append(uv_data)
            
        # 导出三角形
        bm.faces.ensure_lookup_table()
        for face in bm.faces:
            if len(face.verts) == 3:
                triangles.extend([v.index for v in face.verts])
            elif len(face.verts) == 4:
                # 四边形三角化
                v0, v1, v2, v3 = face.verts
                triangles.extend([v0.index, v1.index, v2.index])
                triangles.extend([v0.index, v2.index, v3.index])
                
        bm.free()
        
        return MeshData(
            vertices=vertices,
            normals=normals,
            uvs=uv_layers,
            triangles=triangles,
            name=mesh.name
        )
        
    def _export_material(self, obj) -> Optional[MaterialData]:
        """导出材质数据"""
        if not obj.data.materials:
            return None
            
        mat = obj.data.materials[0]
        
        # 基本材质属性
        diffuse_color = list(mat.diffuse_color)
        specular_color = list(mat.specular_color)
        
        # PBR属性
        roughness = getattr(mat, 'roughness', 0.5)
        metallic = getattr(mat, 'metallic', 0.0)
        
        # 纹理
        textures = {}
        for node in mat.node_tree.nodes if hasattr(mat, 'node_tree') and mat.node_tree else []:
            if node.type == 'TEX_IMAGE' and node.image:
                textures[node.name] = node.image.filepath
                
        return MaterialData(
            name=mat.name,
            diffuse_color=diffuse_color,
            specular_color=specular_color,
            roughness=roughness,
            metallic=metallic,
            textures=textures
        )

class PrismaBlenderPanel(bpy.types.Panel):
    """Blender侧边栏面板"""
    bl_label = "Prisma Engine"
    bl_idname = "VIEW3D_PT_prisma_engine"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Prisma"
    
    def draw(self, context):
        layout = self.layout
        scene = context.scene
        prisma_props = scene.prisma_props
        
        # 连接状态
        row = layout.row()
        if prisma_props.connected:
            row.label(text="✓ Connected to Prisma Engine", icon='CHECKMARK')
        else:
            row.label(text="✗ Not Connected", icon='CANCEL')
            
        # 连接按钮
        row = layout.row()
        if not prisma_props.connected:
            row.operator("prisma.connect", text="Connect", icon='PLAY')
        else:
            row.operator("prisma.disconnect", text="Disconnect", icon='PAUSE')
            
        # 同步选项
        box = layout.box()
        box.label(text="Sync Options", icon='SETTINGS')
        
        row = box.row()
        row.prop(prisma_props, "auto_sync")
        
        row = box.row()
        row.prop(prisma_props, "sync_interval")
        
        if not prisma_props.auto_sync:
            row = box.row()
            row.operator("prisma.sync_now", text="Sync Now", icon='FILE_REFRESH')
            
        # 导出选项
        box = layout.box()
        box.label(text="Export Options", icon='EXPORT')
        
        row = box.row()
        row.prop(prisma_props, "export_meshes")
        
        row = box.row()
        row.prop(prisma_props, "export_materials")
        
        row = box.row()
        row.prop(prisma_props, "export_textures")
        
        # 日志
        box = layout.box()
        box.label(text="Log", icon='TEXT')
        
        for log_entry in prisma_props.log_entries[-5:]:
            box.label(text=log_entry)

class PrismaConnectOperator(bpy.types.Operator):
    """连接Prisma Engine操作符"""
    bl_idname = "prisma.connect"
    bl_label = "Connect to Prisma Engine"
    
    def execute(self, context):
        scene = context.scene
        prisma_props = scene.prisma_props
        
        try:
            prisma_props.ipc_server.start()
            prisma_props.connected = True
            prisma_props.add_log("Connected to Prisma Engine")
            self.report({'INFO'}, "Connected to Prisma Engine")
        except Exception as e:
            prisma_props.add_log(f"Connection failed: {e}")
            self.report({'ERROR'}, f"Connection failed: {e}")
            
        return {'FINISHED'}

class PrismaDisconnectOperator(bpy.types.Operator):
    """断开Prisma Engine连接操作符"""
    bl_idname = "prisma.disconnect"
    bl_label = "Disconnect from Prisma Engine"
    
    def execute(self, context):
        scene = context.scene
        prisma_props = scene.prisma_props
        
        try:
            prisma_props.ipc_server.stop()
            prisma_props.connected = False
            prisma_props.add_log("Disconnected from Prisma Engine")
            self.report({'INFO'}, "Disconnected from Prisma Engine")
        except Exception as e:
            prisma_props.add_log(f"Disconnection error: {e}")
            self.report({'ERROR'}, f"Disconnection error: {e}")
            
        return {'FINISHED'}

class PrismaSyncNowOperator(bpy.types.Operator):
    """立即同步操作符"""
    bl_idname = "prisma.sync_now"
    bl_label = "Sync Now"
    
    def execute(self, context):
        scene = context.scene
        prisma_props = scene.prisma_props
        
        if prisma_props.connected:
            exporter = BlenderSceneExporter()
            scene_data = exporter.export_scene()
            prisma_props.ipc_server.send_message(MessageType.SCENE_UPDATE, scene_data)
            prisma_props.add_log("Scene synced to Prisma Engine")
            self.report({'INFO'}, "Scene synced")
        else:
            self.report({'WARNING'}, "Not connected to Prisma Engine")
            
        return {'FINISHED'}

class PrismaProperties(bpy.types.PropertyGroup):
    """Prisma插件属性"""
    
    connected: bpy.props.BoolProperty(
        name="Connected",
        description="Connection status with Prisma Engine",
        default=False
    )
    
    auto_sync: bpy.props.BoolProperty(
        name="Auto Sync",
        description="Automatically sync changes to Prisma Engine",
        default=True
    )
    
    sync_interval: bpy.props.IntProperty(
        name="Sync Interval",
        description="Sync interval in seconds",
        default=1,
        min=1,
        max=60
    )
    
    export_meshes: bpy.props.BoolProperty(
        name="Export Meshes",
        description="Export mesh geometry",
        default=True
    )
    
    export_materials: bpy.props.BoolProperty(
        name="Export Materials",
        description="Export material data",
        default=True
    )
    
    export_textures: bpy.props.BoolProperty(
        name="Export Textures",
        description="Export texture references",
        default=True
    )
    
    log_entries: bpy.props.CollectionProperty(
        type=bpy.types.PropertyGroup
    )
    
    def add_log(self, message: str):
        """添加日志条目"""
        # 简化实现，实际使用时需要更复杂的日志管理
        if hasattr(self, '_log_list'):
            self._log_list.append(message)
            if len(self._log_list) > 100:
                self._log_list.pop(0)
        else:
            self._log_list = [message]

# Blender插件注册
classes = [
    PrismaProperties,
    PrismaBlenderPanel,
    PrismaConnectOperator,
    PrismaDisconnectOperator,
    PrismaSyncNowOperator,
]

def register():
    """注册插件"""
    for cls in classes:
        bpy.utils.register_class(cls)
        
    bpy.types.Scene.prisma_props = bpy.props.PointerProperty(type=PrismaProperties)
    
    # 初始化IPC服务器
    scene = bpy.context.scene
    scene.prisma_props.ipc_server = PrismaIPCServer()
    scene.prisma_props._log_list = []
    
    print("Prisma Blender Addon registered")

def unregister():
    """注销插件"""
    # 停止IPC服务器
    scene = bpy.context.scene
    if hasattr(scene, 'prisma_props') and hasattr(scene.prisma_props, 'ipc_server'):
        scene.prisma_props.ipc_server.stop()
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
        
    del bpy.types.Scene.prisma_props
    
    print("Prisma Blender Addon unregistered")

if __name__ == "__main__":
    register()