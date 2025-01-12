package com.nay;

import com.nay.model.Entry;

import javax.swing.*;
import java.util.Arrays;

public class Jarabel extends JFrame {
    public static boolean searchAllDrives;

    public static void main(String[] args) {
        searchAllDrives = Arrays.asList(args).contains("-a");
        Entry entry = new Entry();
    }
}
