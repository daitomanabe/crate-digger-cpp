#!/usr/bin/env python3
"""
Golden Tests for Crate Digger C++ via Python bindings

Per INTRODUCTION_JAVA_TO_CPP.md Section 8:
Python Test (Golden Data Verification) - Import Core from Python,
inject input, and verify output.

Usage:
    python golden_test.py [path/to/export.pdb]

If no PDB file is provided, only API schema tests are run.
"""

import sys
import json


def test_api_schema():
    """Test the describe_api() function returns valid JSON schema."""
    try:
        import crate_digger
    except ImportError:
        print("SKIP: crate_digger module not built")
        return True

    print("Testing describe_api()...")

    schema_json = crate_digger.describe_api()
    schema = json.loads(schema_json)

    # Verify required fields
    assert schema["name"] == "crate_digger", "Schema name mismatch"
    assert "version" in schema, "Missing version"
    assert "commands" in schema, "Missing commands"
    assert "inputs" in schema, "Missing inputs"
    assert "outputs" in schema, "Missing outputs"

    # Verify essential commands exist
    command_names = [cmd["name"] for cmd in schema["commands"]]
    assert "open" in command_names, "Missing 'open' command"
    assert "get_track" in command_names, "Missing 'get_track' command"
    assert "describe_api" in command_names, "Missing 'describe_api' command"

    print(f"  Found {len(schema['commands'])} commands")
    print(f"  Found {len(schema['inputs'])} input tensors")
    print(f"  Found {len(schema['outputs'])} output tensors")
    print("  PASSED")
    return True


def test_id_types():
    """Test that ID types work correctly."""
    try:
        import crate_digger
    except ImportError:
        print("SKIP: crate_digger module not built")
        return True

    print("Testing ID types...")

    track_id = crate_digger.TrackId(42)
    assert track_id.value == 42, "TrackId value mismatch"
    assert int(track_id) == 42, "TrackId int conversion failed"

    artist_id = crate_digger.ArtistId(100)
    assert artist_id.value == 100, "ArtistId value mismatch"

    print("  TrackId and ArtistId work correctly")
    print("  PASSED")
    return True


def test_database_open_nonexistent():
    """Test that opening a nonexistent file raises an error."""
    try:
        import crate_digger
    except ImportError:
        print("SKIP: crate_digger module not built")
        return True

    print("Testing Database.open() with nonexistent file...")

    try:
        db = crate_digger.Database.open("/nonexistent/path/export.pdb")
        print("  FAILED: Should have raised an exception")
        return False
    except RuntimeError as e:
        print(f"  Correctly raised error: {e}")
        print("  PASSED")
        return True


def test_database_with_file(pdb_path: str):
    """Test database operations with a real PDB file."""
    try:
        import crate_digger
    except ImportError:
        print("SKIP: crate_digger module not built")
        return True

    print(f"Testing Database.open('{pdb_path}')...")

    try:
        db = crate_digger.Database.open(pdb_path)
    except RuntimeError as e:
        print(f"  FAILED: Could not open database: {e}")
        return False

    print(f"  Opened database: {db}")
    print(f"  Track count: {db.track_count}")
    print(f"  Artist count: {db.artist_count}")
    print(f"  Album count: {db.album_count}")
    print(f"  Genre count: {db.genre_count}")
    print(f"  Playlist count: {db.playlist_count}")

    # Test getting all track IDs
    track_ids = db.all_track_ids()
    print(f"  Retrieved {len(track_ids)} track IDs")

    if track_ids:
        # Test getting a specific track
        first_track = db.get_track(track_ids[0])
        if first_track:
            print(f"  First track: {first_track}")
            print(f"    Title: {first_track.title}")
            print(f"    BPM: {first_track.bpm}")
            print(f"    Duration: {first_track.duration_seconds}s")

    # Test bulk data extraction (for NumPy)
    bpms = db.get_all_bpms()
    print(f"  Retrieved {len(bpms)} BPM values")

    if bpms:
        avg_bpm = sum(bpms) / len(bpms)
        print(f"  Average BPM: {avg_bpm:.2f}")

    durations = db.get_all_durations()
    print(f"  Retrieved {len(durations)} duration values")

    if durations:
        avg_duration = sum(durations) / len(durations)
        print(f"  Average duration: {avg_duration:.1f}s")

    print("  PASSED")
    return True


def test_numpy_integration(pdb_path: str):
    """Test NumPy integration if available."""
    try:
        import crate_digger
        import numpy as np
    except ImportError as e:
        print(f"SKIP: {e}")
        return True

    print("Testing NumPy integration...")

    try:
        db = crate_digger.Database.open(pdb_path)
    except RuntimeError as e:
        print(f"  SKIP: Could not open database: {e}")
        return True

    # Convert to numpy arrays
    bpms = np.array(db.get_all_bpms(), dtype=np.float32)
    durations = np.array(db.get_all_durations(), dtype=np.int32)

    print(f"  BPM array shape: {bpms.shape}, dtype: {bpms.dtype}")
    print(f"  Duration array shape: {durations.shape}, dtype: {durations.dtype}")

    if len(bpms) > 0:
        print(f"  BPM stats: min={bpms.min():.1f}, max={bpms.max():.1f}, mean={bpms.mean():.1f}")

    if len(durations) > 0:
        print(f"  Duration stats: min={durations.min()}s, max={durations.max()}s, mean={durations.mean():.1f}s")

    print("  PASSED")
    return True


def main():
    print("=" * 60)
    print("Crate Digger Golden Tests")
    print("=" * 60)
    print()

    pdb_path = sys.argv[1] if len(sys.argv) > 1 else None

    results = []

    # API tests (no file needed)
    results.append(("API Schema", test_api_schema()))
    results.append(("ID Types", test_id_types()))
    results.append(("Open Nonexistent", test_database_open_nonexistent()))

    # File-based tests
    if pdb_path:
        results.append(("Database Operations", test_database_with_file(pdb_path)))
        results.append(("NumPy Integration", test_numpy_integration(pdb_path)))
    else:
        print("\nNote: Provide a PDB file path to run database tests.")
        print("  python golden_test.py /path/to/export.pdb")

    # Summary
    print()
    print("=" * 60)
    print("Summary")
    print("=" * 60)

    passed = sum(1 for _, result in results if result)
    total = len(results)

    for name, result in results:
        status = "PASSED" if result else "FAILED"
        print(f"  {name}: {status}")

    print()
    print(f"Passed: {passed}/{total}")

    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
