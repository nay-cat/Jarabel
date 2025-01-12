package com.nay;

import com.nay.model.Entry;

import javax.swing.*;
import java.util.Arrays;

public class Jarabel extends JFrame {
    public static boolean searchAllDrives;
    public static boolean here;


    public static void main(String[] args) {
        searchAllDrives = Arrays.asList(args).contains("-a");
        here = Arrays.asList(args).contains("-here");
        Entry entry = new Entry();
    }
}
