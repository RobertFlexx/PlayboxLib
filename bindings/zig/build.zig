const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const include_dir = b.path("../../include");
    const lib_dir = b.path("../../build/lib");

    const playbox_mod = b.createModule(.{
        .root_source_file = b.path("playbox.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    playbox_mod.addIncludePath(include_dir);

    const demo_mod = b.createModule(.{
        .root_source_file = b.path("demo.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .imports = &.{
            .{ .name = "playbox", .module = playbox_mod },
        },
    });
    demo_mod.addIncludePath(include_dir);
    demo_mod.addLibraryPath(lib_dir);
    demo_mod.linkSystemLibrary("playbox", .{});
    demo_mod.linkSystemLibrary("m", .{});
    demo_mod.addRPath(lib_dir);

    const demo = b.addExecutable(.{
        .name = "pb_zig_demo",
        .root_module = demo_mod,
    });

    b.installArtifact(demo);

    const run = b.addRunArtifact(demo);
    const run_step = b.step("run", "Run the Zig Playbox demo");
    run_step.dependOn(&run.step);
}
