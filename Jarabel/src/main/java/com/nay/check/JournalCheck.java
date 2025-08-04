package com.nay.check;

import com.nay.api.Kernel32;
import com.nay.model.Check;
import com.nay.model.JarRecord;
import com.nay.structure.ReadUSNJournalDataV0;
import com.nay.structure.USNJournalData;
import com.nay.structure.USNRecord;
import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.WString;
import com.sun.jna.platform.win32.WinBase;
import com.sun.jna.platform.win32.WinNT;
import com.sun.jna.platform.win32.Winioctl;
import com.sun.jna.platform.win32.WinioctlUtil;
import com.sun.jna.ptr.IntByReference;

import java.util.ArrayList;
import java.util.List;

public class JournalCheck extends Check {

    public JournalCheck() {
        this.name = "Journal Check";
    }

    public static List<JarRecord> jarRecordsList = new ArrayList<>();

    static final int FSCTL_QUERY_USN_JOURNAL =
            WinioctlUtil.CTL_CODE(Winioctl.FILE_DEVICE_FILE_SYSTEM, 61,
                    Winioctl.METHOD_BUFFERED, Winioctl.FILE_ANY_ACCESS);

    static final int FSCTL_READ_USN_JOURNAL =
            WinioctlUtil.CTL_CODE(Winioctl.FILE_DEVICE_FILE_SYSTEM, 46,
                    Winioctl.METHOD_NEITHER, Winioctl.FILE_ANY_ACCESS);

    private static final int BUF_LEN = (int) Math.pow(2, 16384);

    @Override
    public void execute() {
        WString volume = new WString("\\\\.\\c:");
        WinNT.HANDLE hVol = Kernel32.INSTANCE.CreateFileW(volume, 0x80000000 | 0x40000000, 0x00000001 | 0x00000002,
                null, 3, 0, null);

        if (WinBase.INVALID_HANDLE_VALUE.equals(hVol)) {
            System.err.println("CreateFile failed: " + Kernel32.INSTANCE.GetLastError());
            return;
        }

        USNJournalData journalData = new USNJournalData();
        IntByReference bytesReturned = new IntByReference();

        boolean queryResult = Kernel32.INSTANCE.DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, null, 0,
                journalData.getPointer(), journalData.size(), bytesReturned, null);

        if (!queryResult) {
            System.err.println("Query journal failed: " + Kernel32.INSTANCE.GetLastError());
            Kernel32.INSTANCE.CloseHandle(hVol);
            return;
        }

        journalData.read();
        System.out.printf("Journal ID: "+ journalData.UsnJournalID);

        ReadUSNJournalDataV0 readData = new ReadUSNJournalDataV0();
        readData.StartUsn = journalData.FirstUsn;
        readData.ReasonMask = 0x00003400;
        readData.ReturnOnlyOnClose = 0;
        readData.Timeout = 0;
        readData.BytesToWaitFor = 0;
        readData.UsnJournalID = journalData.UsnJournalID;
        readData.write();

        Memory buffer = new Memory(BUF_LEN);
        boolean readResult = Kernel32.INSTANCE.DeviceIoControl(
                hVol, FSCTL_READ_USN_JOURNAL,
                readData.getPointer(), readData.size(),
                buffer, (int) buffer.size(),
                bytesReturned, null
        );

        if (!readResult) {
            int error = Kernel32.INSTANCE.GetLastError();
            System.err.printf("error", error, Kernel32.INSTANCE.GetLastError());
            Kernel32.INSTANCE.CloseHandle(hVol);
            return;
        }

        if (bytesReturned.getValue() <= 8) {
            System.err.println("error with device control");
            Kernel32.INSTANCE.CloseHandle(hVol);
            return;
        }

        Pointer currentRecord = buffer.share(8);
        int remainingBytes = bytesReturned.getValue() - 8;
        int totalBytes = remainingBytes;

        while (remainingBytes > 8) {
            USNRecord record = new USNRecord(currentRecord);
            record.read();

            if (record.RecordLength < 8 || record.RecordLength > remainingBytes) {
                System.err.printf("corrupted record %d (Remaining Bytes: %d)\n", record.RecordLength, remainingBytes);
                break;
            }

            String name = new String(record.FileName, 0, record.FileNameLength / 2).trim();
            double progress = ((double) (totalBytes - remainingBytes) / totalBytes) * 100;

            int barLength = 30;
            int filled = (int) (progress / 100 * barLength);
            StringBuilder bar = new StringBuilder();
            for (int i = 0; i < barLength; i++) {
                bar.append(i < filled ? "I" : "-");
            }


            System.out.printf("Progress: %.2f%% [%s]\n", progress, bar.toString());

            if (name.contains(".jar")) {
                JarRecord jarRecord = new JarRecord(record.Usn, name, record.FileReferenceNumber, record.TimeStamp, record.Reason);
                jarRecordsList.add(jarRecord);
            }

            remainingBytes -= record.RecordLength;
            if (remainingBytes >= 0) {
                currentRecord = currentRecord.share(record.RecordLength);
            } else {
                System.err.println("Finished");
                break;
            }

            readData.StartUsn = buffer.getLong(0);
        }


        Kernel32.INSTANCE.CloseHandle(hVol);
    }
}
