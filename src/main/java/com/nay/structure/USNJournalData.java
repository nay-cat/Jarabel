package com.nay.structure;

import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class USNJournalData extends Structure {
    public long UsnJournalID;
    public long FirstUsn;
    public long NextUsn;
    public long LowestValidUsn;
    public long MaxUsn;
    public long MaximumSize;
    public long AllocationDelta;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("UsnJournalID", "FirstUsn", "NextUsn", "LowestValidUsn", "MaxUsn", "MaximumSize", "AllocationDelta");
    }
}