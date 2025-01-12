package com.nay.model;

import com.nay.app.Application;
import com.nay.inject.EntryModule;
import com.nay.service.Service;
import team.unnamed.inject.Inject;
import team.unnamed.inject.Injector;

import javax.swing.*;
import java.util.List;

public class Entry {

    @Inject
    private List<Service> services;

    public Entry(){
        System.out.println("Loading Jarabel");
        Injector injector = Injector.create(new EntryModule());
        injector.injectMembers(this);
        services.forEach(Service::start);

        SwingUtilities.invokeLater(() -> new Application().createAndShowGUI());
    }
}
