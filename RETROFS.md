# RetroFS Partition Type GUID

**Name:** RetroFS Partition

**Use:** Identifies a GPT partition formatted with the RetroFS filesystem.

**GUID (UUID):** `4DEC1156-FEC8-4495-854B-20D888E21AF0`

**Format:** UUID v4 (randomly generated, permanently reserved for RetroFS).

**Endianess:** Stored in GPT entries as per UEFI specification (little‑endian in the first 3 fields).

**Operating System Usage:**

* Retro Rocket will use this type GUID to automatically probe and mount RetroFS volumes.
* No other operating systems are expected to recognise this type by default.

---

## Example in GPT Entry

```
Partition GUID code: 4DEC1156-FEC8-4495-854B-20D888E21AF0 (RetroFS partition)
Partition unique GUID: [per‑volume unique UUID]
First sector: N
Last sector: M
Partition size: [M‑N+1] sectors
Partition name: 'RetroFS'
```

---

## Kernel Header

```c
/**
 * @brief RetroFS GPT Partition Type GUID
 * UUID v4 reserved for Retro Rocket RetroFS partitions
 */
#define RFS_GPT_GUID  "4DEC1156-FEC8-4495-854B-20D888E21AF0"
```

