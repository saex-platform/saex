using System.Buffers.Binary;

namespace Saex.Contracts;

// Local D1 conformance frame. Not the game network protocol or production IPC.
public static class FixtureCodec
{
    public const int HeaderBytes = 12;
    public const int FrameBytes = HeaderBytes + FoundationContract.PayloadBytes;

    public static byte[] Encode(EntityRef entity)
    {
        if (!entity.IsValid) throw new ArgumentException("Invalid EntityRef", nameof(entity));
        var result = new byte[FrameBytes];
        BinaryPrimitives.WriteUInt32LittleEndian(result, FoundationContract.FixtureMagic);
        BinaryPrimitives.WriteUInt16LittleEndian(result.AsSpan(4), FoundationContract.FixtureVersion);
        BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(8), FoundationContract.PayloadBytes);
        BinaryPrimitives.WriteUInt64LittleEndian(result.AsSpan(12), entity.WorldId.Value);
        BinaryPrimitives.WriteUInt64LittleEndian(result.AsSpan(20), entity.WorldEpoch.Value);
        BinaryPrimitives.WriteUInt64LittleEndian(result.AsSpan(28), entity.EntityId.Value);
        BinaryPrimitives.WriteUInt64LittleEndian(result.AsSpan(36), entity.Generation.Value);
        return result;
    }

    public static EntityRef Decode(ReadOnlySpan<byte> frame)
    {
        if (frame.Length != FrameBytes) throw new InvalidDataException("invalid_length");
        if (BinaryPrimitives.ReadUInt32LittleEndian(frame) != FoundationContract.FixtureMagic)
            throw new InvalidDataException("bad_magic");
        if (BinaryPrimitives.ReadUInt16LittleEndian(frame[4..]) != FoundationContract.FixtureVersion)
            throw new InvalidDataException("unsupported_version");
        if (BinaryPrimitives.ReadUInt16LittleEndian(frame[6..]) != 0) throw new InvalidDataException("reserved_bits");
        if (BinaryPrimitives.ReadUInt32LittleEndian(frame[8..]) != FoundationContract.PayloadBytes)
            throw new InvalidDataException("invalid_length");
        var entity = new EntityRef(new(BinaryPrimitives.ReadUInt64LittleEndian(frame[12..])),
            new(BinaryPrimitives.ReadUInt64LittleEndian(frame[20..])),
            new(BinaryPrimitives.ReadUInt64LittleEndian(frame[28..])),
            new(BinaryPrimitives.ReadUInt64LittleEndian(frame[36..])));
        if (!entity.IsValid) throw new InvalidDataException("invalid_entity");
        return entity;
    }
}
