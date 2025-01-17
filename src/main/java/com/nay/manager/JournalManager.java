package com.nay.manager;

import com.nay.check.JournalCheck;
import com.nay.model.JarRecord;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class JournalManager {


    public static List<String> getProcessJarRecords(List<JarRecord> records) {
        Map<Long, List<JarRecord>> groupedByFRN = records.stream()
                .collect(Collectors.groupingBy(JarRecord::getFRN));

        List<String> result = new ArrayList<>();

        for (Map.Entry<Long, List<JarRecord>> entry : groupedByFRN.entrySet()) {
            List<JarRecord> group = entry.getValue();
            if (group.size() > 1) {
                StringBuilder duplicates = new StringBuilder();
                for (JarRecord record : group) {
                    duplicates.append(formatRecord(record)).append("\n");
                }
                result.add(duplicates.toString().trim());
            } else {
                result.add(formatRecord(group.get(0)));
            }
        }

        return Collections.unmodifiableList(result);
    }

    private static String formatRecord(JarRecord record) {
        return String.format("USN: %d - Name: %s FRN: %s Reason: %s",
                record.getUSN(),
                record.getName(),
                record.getFRN(),
                record.getTranslatedReason());
    }
}
