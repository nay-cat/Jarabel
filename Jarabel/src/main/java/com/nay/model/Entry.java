package com.nay.model;

import com.nay.Jarabel;
import com.nay.app.Application;
import com.nay.check.JournalCheck;
import com.nay.inject.EntryModule;
import com.nay.service.Service;
import team.unnamed.inject.Inject;
import team.unnamed.inject.Injector;

import javax.swing.*;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class Entry {

    @Inject
    private List<Service> services;

    public Entry() {
        System.out.println("Loading Jarabel");

        Injector injector = Injector.create(new EntryModule());
        injector.injectMembers(this);

        ExecutorService executorService = Executors.newCachedThreadPool();

        for (Service service : services) {
            executorService.submit(service::start);
        }

        if (Jarabel.journal) {
            executorService.submit(() -> {
                JournalCheck journalCheck = new JournalCheck();
                journalCheck.execute();
            });
        }

        executorService.shutdown();
        try {
            executorService.awaitTermination(Long.MAX_VALUE, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            System.err.println("Execution interrupted: " + e.getMessage());
        }

        SwingUtilities.invokeLater(() -> new Application().createAndShowGUI());
    }
}