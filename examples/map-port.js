#!/usr/bin/env gjs
//
// Copyright (c) 2021, Sonny Piers <sonny@fastmail.net>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//         SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.

const { GUPnPIgd, GLib } = imports.gi;
const { system } = imports;

const { SimpleIgd } = GUPnPIgd;
const { MainLoop } = GLib;

const mainloop = new MainLoop(null, false);

const protocol = "TCP";
const external_port = 0; // 0 means local_port if available otherwise random port
const local_ip = ARGV[0];
const local_port = ARGV[1];
const lease_duration = 3000; // auto-renewed until port mapping is removed or process exits
const description = "gupnp-igd JavaScript example";

let exit_code = 0;

if (!local_ip || !local_port) {
  print("Usage: ./map-port.js IP PORT");
  system.exit(1);
}

const simpleIgd = new SimpleIgd();
simpleIgd.connect(
  "error-mapping-port",
  (self, error, proto, external_port, local_ip, local_port, description) => {
    printerr(error);
    exit_code = 1;
    mainloop.quit();
  }
);

simpleIgd.connect(
  "mapped-external-port",
  (
    self,
    proto,
    external_ip,
    replaces_external_ip,
    external_port,
    local_ip,
    local_port,
    description
  ) => {
    print(
      `success ${external_ip}:${external_port} -> ${local_ip}:${local_port}`
    );
  }
);

simpleIgd.add_port(
  protocol,
  external_port,
  local_ip,
  local_port,
  lease_duration,
  description
);

mainloop.run();
system.exit(exit_code);
