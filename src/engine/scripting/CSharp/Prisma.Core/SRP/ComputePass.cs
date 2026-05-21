namespace Prisma.SRP;

public sealed class ComputePass : IDisposable
{
    public string Name { get; }
    public ComputePipeline Pipeline { get; }
    public int GroupX { get; }
    public int GroupY { get; }
    public int GroupZ { get; }

    public ComputePass(string name, ComputePipeline pipeline, int groupsX, int groupsY, int groupsZ)
    {
        Name = name;
        Pipeline = pipeline;
        GroupX = groupsX;
        GroupY = groupsY;
        GroupZ = groupsZ;
    }

    public void Execute(CommandBuffer cmd)
    {
        cmd.SetComputePipeline(Pipeline);
        cmd.Dispatch(GroupX, GroupY, GroupZ);
    }

    public void Dispose()
    {
        Pipeline.Dispose();
    }
}
