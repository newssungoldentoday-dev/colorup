#!/usr/bin/env node
const fs = require('fs');
const { parse, syscall } = require('../src/syscall.js');
const file = process.argv[2];
if(!file){ console.log('Usage: colorup <file.colorup>'); process.exit(1); }
const content = fs.readFileSync(file,'utf8');
console.log(parse(content));
