package com.nay.structure;

import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class ReadUSNJournalDataV0 extends Structure {
    public long StartUsn;
    public int ReasonMask;
    public int ReturnOnlyOnClose;
    public long Timeout;
    public long BytesToWaitFor;
    public long UsnJournalID;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("StartUsn", "ReasonMask", "ReturnOnlyOnClose", "Timeout", "BytesToWaitFor", "UsnJournalID");
    }
}