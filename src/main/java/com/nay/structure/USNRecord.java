package com.nay.structure;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class USNRecord extends Structure {
    public USNRecord(Pointer m) {
        super.useMemory(m);
        this.read();
    }

    public int RecordLength;
    public short MajorVersion;
    public short MinorVersion;
    public long FileReferenceNumber;
    public long ParentFileReferenceNumber;
    public long Usn;
    public long TimeStamp;
    public int Reason;
    public int SourceInfo;
    public int SecurityId;
    public int FileAttributes;
    public short FileNameLength;
    public short FileNameOffset;
    public char[] FileName = new char[1024];

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("RecordLength", "MajorVersion", "MinorVersion", "FileReferenceNumber",
                "ParentFileReferenceNumber", "Usn", "TimeStamp", "Reason", "SourceInfo",
                "SecurityId", "FileAttributes", "FileNameLength", "FileNameOffset", "FileName");
    }
}
