# Playbox replay format (.pbr)

This document defines the on-disk format used by Playbox to record and replay application runs.

A `.pbr` file captures:

* exactly when updates happened (delta time per frame)
* exactly which input events occurred
* optional metadata that makes playback deterministic

The intent is simple:

A replay should behave the same way every time it is played back.

File extension: `.pbr`
Encoding: binary
Endianness: little-endian

---

## High-level structure

A replay file is made of two parts:

1. A small header (version and metadata)
2. A sequence of frames, written in order

```
[ header ]
[ frame 0 ]
[ frame 1 ]
[ frame 2 ]
...
```

Each frame corresponds to one update tick of the original run.

---

## Versioning

Two file versions exist.

### PBR2 (current)

Magic bytes: `PBR2`

Adds a metadata header that includes:

* RNG seed
* initial terminal size

This version should be used for all new recordings.

### PBR1 (legacy)

Magic bytes: `PBR1`

No metadata header.

Playback still works, but:

* RNG seed is assumed to be 0
* initial terminal size is unknown

---

## PBR2 header layout

The file always starts with the following structure:

| Offset | Size    | Field                   |
| ------ | ------- | ----------------------- |
| 0x00   | 4 bytes | magic string (`PBR2`)   |
| 0x04   | u32     | RNG seed                |
| 0x08   | u32     | initial terminal width  |
| 0x0C   | u32     | initial terminal height |

All values are stored as little-endian.

### Header field meanings

**seed**
Used to make gameplay deterministic.
If your game uses randomness, initialize your RNG using this value during replay.

**initial_w / initial_h**
The terminal size at the start of recording.
These values are informational and can be used to:

* warn the user if the replay is played at a different size
* resize the application window on replay start

---

## Frame layout (all versions)

After the header, the file contains a sequence of frames.

Each frame represents one update step from the original run.

| Field       | Type  | Description                          |
| ----------- | ----- | ------------------------------------ |
| dt          | f64   | delta time passed to update          |
| event_count | u32   | number of input events in this frame |
| events      | array | serialized input events              |

Events are stored in the exact order they were collected.

---

## Event serialization

Every event begins with a `type` field that identifies which payload follows.

| Field | Type                  |
| ----- | --------------------- |
| type  | u32 (`pb_event_type`) |

The remaining data depends on the event type.

---

### PB_EVENT_KEY

Represents a key press or release.

| Field     | Type |
| --------- | ---- |
| key       | u32  |
| codepoint | u32  |
| alt       | u32  |
| ctrl      | u32  |
| shift     | u32  |
| pressed   | u32  |

---

### PB_EVENT_TEXT

Represents a text input event.

| Field     | Type |
| --------- | ---- |
| codepoint | u32  |

---

### PB_EVENT_MOUSE

Represents mouse movement, clicks, or wheel input.

| Field   | Type |
| ------- | ---- |
| x       | i32  |
| y       | i32  |
| button  | u32  |
| pressed | u32  |
| wheel   | i32  |
| shift   | u32  |
| alt     | u32  |
| ctrl    | u32  |

---

### PB_EVENT_RESIZE

Represents a terminal resize event.

| Field  | Type |
| ------ | ---- |
| width  | i32  |
| height | i32  |

---

### PB_EVENT_NONE

No additional payload.

---

### PB_EVENT_QUIT

No additional payload.

---

## Replay behavior

* During replay, live keyboard and mouse input is ignored.
* Input events are applied exactly as recorded.
* Resize events are replayed as part of the input stream.
* The `dt` value for each frame comes from the file, not real time.

This means your update loop receives the same timing and input sequence as the original run.

---

## Deterministic playback

If your application uses randomness:

1. Read the replay seed from the replay header
2. Initialize your RNG with it
3. Avoid reading external entropy during replay

For example:

```
seed = pb_app_replay_seed(app)
rng_init(seed)
```

If these rules are followed, replays will remain stable across machines, runs, and platforms.
