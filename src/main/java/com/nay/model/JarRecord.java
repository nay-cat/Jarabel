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
        if ((journalReason & 0x00000002) != 0) reasons.add("Data truncated");
        if ((journalReason & 0x00000004) != 0) reasons.add("File created");
        if ((journalReason & 0x00000010) != 0) reasons.add("File deleted");
        if ((journalReason & 0x00000020) != 0) reasons.add("Extended attributes changed");
        if ((journalReason & 0x00000200) != 0) reasons.add("Rename old name");
        if ((journalReason & 0x00000400) != 0) reasons.add("Rename new name");

        return reasons.isEmpty() ? "Unknown reason" : String.join(" + ", reasons);
    }

    public String getParsedDate() {
        Date date = new Date(timeStamp);
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return formatter.format(date);
    }
}
