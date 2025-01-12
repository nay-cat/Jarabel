package com.nay.inject.module;

import com.nay.service.Service;
import com.nay.service.impl.CheckService;
import team.unnamed.inject.AbstractModule;

public class ServiceModule extends AbstractModule {

    @Override
    protected void configure() {
        System.out.println("Loaded ServiceModule");
        this.multibind(Service.class)
                .asList()
                .to(CheckService.class)
                .singleton();
    }

}
