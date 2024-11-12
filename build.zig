const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "pnr",
        .target = b.host,
        .optimize = b.standardOptimizeOption(.{}),
    });

    const files = [_][]const u8{
        "lval.c",
        "mpc.c",
        "main.c",
    };

    exe.addCSourceFiles(.{
        .files = &files,
        .flags = &[_][]const u8{},
    });

    exe.linkLibC();
    exe.linkSystemLibrary("edit");
    b.installArtifact(exe);
}
