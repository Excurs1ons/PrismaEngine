namespace Prisma.SRP;

/// <summary>
/// GPU 命令录制器。所有渲染/计算命令通过此类发出，不直接访问 Interop.API。
/// 类似 Unity ScriptableRenderContext + CommandBuffer 的合并简化版。
/// </summary>
public sealed class CommandBuffer
{
    /// <summary>开始新帧。（对应 C++ RenderSystem 的帧开始）</summary>
    public void BeginFrame()
    {
        unsafe { Interop.API.SrpBeginFrame(); }
    }

    /// <summary>结束当前帧。</summary>
    public void EndFrame()
    {
        unsafe { Interop.API.SrpEndFrame(); }
    }

    // ================================================================
    // Render Pass
    // ================================================================

    /// <summary>开始渲染通道。</summary>
    public void BeginRenderPass(
        int viewW = 1280, int viewH = 720,
        float depthClear = 1.0f,
        float r = 0, float g = 0, float b = 0, float a = 0)
    {
        unsafe
        {
            uint noRT = 0;
            float cc = 0;
            Interop.API.SrpCmdBeginRenderPass(0, &noRT, 0, &cc, depthClear, viewW, viewH);
        }
    }

    /// <summary>结束渲染通道。</summary>
    public void EndRenderPass()
    {
        unsafe { Interop.API.SrpCmdEndRenderPass(); }
    }

    // ================================================================
    // Pipeline (Graphics)
    // ================================================================

    /// <summary>绑定图形管线。</summary>
    public void BindPipeline(GraphicsPipeline pipeline)
    {
        unsafe { Interop.API.SrpCmdBindPipeline(pipeline.Handle); }
    }

    // ================================================================
    // Compute Pipeline
    // ================================================================

    /// <summary>绑定计算管线。</summary>
    public void SetComputePipeline(ComputePipeline pipeline)
    {
        unsafe { Interop.API.SrpCmdBindComputePipeline(pipeline.Handle); }
    }

    /// <summary>分派计算线程组。</summary>
    public void Dispatch(int groupsX, int groupsY, int groupsZ)
    {
        unsafe { Interop.API.SrpCmdDispatch((uint)groupsX, (uint)groupsY, (uint)groupsZ); }
    }

    /// <summary>绑定计算着色器纹理（采样器 + 纹理）。</summary>
    public void SetComputeTexture(int slot, Texture texture, Sampler sampler)
    {
        unsafe { Interop.API.SrpCmdBindComputeTexture((uint)slot, texture.Handle, sampler.Handle); }
    }

    /// <summary>绑定计算着色器存储映像（读写）。</summary>
    public void SetStorageImage(int slot, Texture texture)
    {
        unsafe { Interop.API.SrpCmdBindStorageImage((uint)slot, texture.Handle); }
    }

    /// <summary>绑定计算着色器存储缓冲区（SSBO）。</summary>
    public void SetStorageBuffer(int slot, Buffer buffer)
    {
        unsafe { Interop.API.SrpCmdBindStorageBuffer((uint)slot, buffer.Handle); }
    }

    // ================================================================
    // Vertex / Index Buffers
    // ================================================================

    /// <summary>绑定顶点缓冲区。</summary>
    public void SetVertexBuffer(Buffer buffer, int slot = 0, int offset = 0)
    {
        unsafe { Interop.API.SrpCmdBindVertexBuffer(buffer.Handle, (uint)slot, (uint)offset); }
    }

    /// <summary>绑定索引缓冲区。</summary>
    public void SetIndexBuffer(Buffer buffer, int offset = 0, bool is32Bit = false)
    {
        unsafe { Interop.API.SrpCmdBindIndexBuffer(buffer.Handle, (uint)offset, is32Bit ? 1 : 0); }
    }

    // ================================================================
    // Viewport / Scissor
    // ================================================================

    /// <summary>设置视口。</summary>
    public void SetViewport(int x, int y, int w, int h)
    {
        unsafe { Interop.API.SrpCmdSetViewport(x, y, w, h); }
    }

    /// <summary>设置裁剪矩形。</summary>
    public void SetScissor(int x, int y, int w, int h)
    {
        unsafe { Interop.API.SrpCmdSetScissor(x, y, w, h); }
    }

    // ================================================================
    // Push Constants
    // ================================================================

    /// <summary>推送常量数据到 GPU。</summary>
    public void PushConstants<T>(T data) where T : unmanaged
    {
        unsafe { Interop.API.SrpCmdPushConstants(0, (uint)sizeof(T), &data); }
    }

    /// <summary>推送常量数据（指针形式）。</summary>
    public void PushConstants(int offset, int size, IntPtr data)
    {
        unsafe { Interop.API.SrpCmdPushConstants((uint)offset, (uint)size, (void*)data); }
    }

    // ================================================================
    // Draw Calls
    // ================================================================

    /// <summary>绘制非索引几何体。</summary>
    public void Draw(int vertexCount, int instanceCount = 1, int firstVertex = 0)
    {
        unsafe { Interop.API.SrpCmdDraw((uint)vertexCount, (uint)instanceCount, (uint)firstVertex); }
    }

    /// <summary>绘制索引几何体。</summary>
    public void DrawIndexed(int indexCount, int instanceCount = 1, int firstIndex = 0, int vertexOffset = 0)
    {
        unsafe { Interop.API.SrpCmdDrawIndexed((uint)indexCount, (uint)instanceCount, (uint)firstIndex, vertexOffset); }
    }

    /// <summary>绘制全屏四边形。</summary>
    public void DrawFullScreenQuad()
    {
        unsafe { Interop.API.SrpCmdDrawFullScreenQuad(); }
    }

    // ================================================================
    // Texture Binding (Graphics)
    // ================================================================

    /// <summary>绑定纹理（图形管线）。</summary>
    public void BindTexture(int slot, Texture texture, Sampler sampler)
    {
        unsafe { Interop.API.SrpCmdBindTexture((uint)slot, texture.Handle, sampler.Handle); }
    }

    /// <summary>将当前渲染目标拷贝到纹理（供后续 Pass 采样）。</summary>
    public void BlitRenderTarget(Texture dst)
    {
        unsafe { Interop.API.SrpCmdBlitRenderTarget(dst.Handle); }
    }
}
