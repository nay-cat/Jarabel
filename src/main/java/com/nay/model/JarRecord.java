package com.nay.model;

import com.sun.jna.platform.win32.Sspi;
import lombok.Getter;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

@Getter
public class JarRecord {

    private final Long USN;
    private final String name;
    private final Long FRN;
    private final Long timeStamp;
    private final int journalReason;

    public JarRecord(Long usn, String name, Long FRN, long timeStamp, int journalReason) {
        this.USN = usn;
        this.name = name;
        this.FRN = FRN;
        this.timeStamp = timeStamp;
        this.journalReason = journalReason;
    }

    public String getTranslatedReason() {
        List<String> reasons = new ArrayList<>();

        if ((journalReason & 0x00000001) != 0) reasons.add("Data overwritten");
        if ((journalReason & 0x00000002) != 0) reasons.add("Data extended");
        if ((journalReason & 0x00000004) != 0) reasons.add("Data truncated");
        if ((journalReason & 0x00000010) != 0) reasons.add("Named data overwritten");
        if ((journalReason & 0x00000020) != 0) reasons.add("Named data extended");
        if ((journalReason & 0x00000040) != 0) reasons.add("Named data truncated");
        if ((journalReason & 0x00000100) != 0) reasons.add("File created");
        if ((journalReason & 0x00000200) != 0) reasons.add("File deleted");
        if ((journalReason & 0x00000400) != 0) reasons.add("Extended attributes changed");
        if ((journalReason & 0x00000800) != 0) reasons.add("Security changed");
        if ((journalReason & 0x00001000) != 0) reasons.add("Rename old name");
        if ((journalReason & 0x00002000) != 0) reasons.add("Rename new name");
        if ((journalReason & 0x00004000) != 0) reasons.add("Indexable attribute changed");
        if ((journalReason & 0x00008000) != 0) reasons.add("Basic info changed");
        if ((journalReason & 0x00010000) != 0) reasons.add("Hard link changed");
        if ((journalReason & 0x00020000) != 0) reasons.add("Compression changed");
        if ((journalReason & 0x00040000) != 0) reasons.add("Encryption changed");
        if ((journalReason & 0x00080000) != 0) reasons.add("Object ID changed");
        if ((journalReason & 0x00100000) != 0) reasons.add("Reparse point changed");
        if ((journalReason & 0x00200000) != 0) reasons.add("Stream changed");
        if ((journalReason & 0x00400000) != 0) reasons.add("Transacted change");
        if ((journalReason & 0x00800000) != 0) reasons.add("Integrity changed");
        if ((journalReason & 0x80000000) != 0) reasons.add("File closed");


        return reasons.isEmpty() ? "Unknown reason" : String.join(" + ", reasons);
    }

    public String getParsedDate() {
        Date date = new Date(timeStamp);
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return formatter.format(date);
    }
}
