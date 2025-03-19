/*

ESP8266 file system builder

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>;

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// -----------------------------------------------------------------------------
// File system builder
// -----------------------------------------------------------------------------

// const { series, src, dest } = require('gulp');
// const htmlmin = require('gulp-htmlmin');
// const cleancss = require('gulp-clean-css');
// const uglify = require('gulp-uglify-es').default;
// const gzip = require('gulp-gzip');
// const del = require('del');
// const inline = require('gulp-inline');
// const inlineImages = require('gulp-css-base64');
// const favicon = require('gulp-base64-favicon');
// const fs = require('fs');

import gulp from 'gulp';
const { series, src, dest } = gulp;
import htmlmin from 'gulp-htmlmin';
import cleancss from 'gulp-clean-css';
import uglify from 'gulp-uglify-es';
import gzip from 'gulp-gzip';
import { deleteAsync } from 'del';
import inline from 'gulp-inline';
import inlineImages from 'gulp-css-base64';
import favicon from 'gulp-base64-favicon';
import fs from 'fs';
import crypto from 'crypto';

function clean(cb) {
    deleteAsync(["dist/*"]);
    cb();
}

function build(cb) {
    cb();
}

function buildfs_inline(cb) {
    return src('html/*.html')
        .pipe(favicon({ src: "html" }))
        .pipe(inline({
            base: 'html/',
            //            js: uglify,
            css: [cleancss, inlineImages],
            //            disabledTypes: ['svg', 'img']
        }))
        .pipe(htmlmin({
            //            collapseWhitespace: true,
            removecomments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(gzip())
        .pipe(dest("dist"));
}

function buildfs_embeded(cb) {
    var source = 'dist/index.html.gz';
    var destination = 'src/index.html.gz.h';
    write_header_file(source, destination, "index_html_gz");

    cb();
}

function buildfs_yarrboard_logo_gz(cb) {
    return src('html/logo-yarrboard.png')
        .pipe(gzip())
        .pipe(dest("dist"));
}

function buildfs_frothfet_logo_gz(cb) {
    return src('html/logo-frothfet.png')
        .pipe(gzip())
        .pipe(dest("dist"));
}

function buildfs_brineomatic_logo_gz(cb) {
    return src('html/logo-brineomatic.png')
        .pipe(gzip())
        .pipe(dest("dist"));
}

function buildfs_logo_embedded(cb) {
    var source = 'dist/logo-yarrboard.png.gz';
    var destination = 'src/logo-yarrboard.png.gz.h';
    write_header_file(source, destination, "logo_yarrboard_gz");

    source = 'dist/logo-brineomatic.png.gz';
    destination = 'src/logo-brineomatic.png.gz.h';
    write_header_file(source, destination, "logo_brineomatic_gz");

    source = 'dist/logo-frothfet.png.gz';
    destination = 'src/logo-frothfet.png.gz.h';
    write_header_file(source, destination, "logo_frothfet_gz");

    cb();
}

function write_header_file(source, destination, name) {
    const wstream = fs.createWriteStream(destination);
    wstream.on('error', function (err) {
        console.log(err);
    });

    const data = fs.readFileSync(source);

    const hashSum = crypto.createHash('sha256');
    hashSum.update(data);
    const hex = hashSum.digest('hex');

    wstream.write(`#define ${name}_len ${data.length}\n`);
    wstream.write(`const char ${name}_sha[] = "${hex}";\n`);
    wstream.write(`const uint8_t ${name}[] = {`);

    for (var i = 0; i < data.length; i++) {
        if (i % 1000 == 0) wstream.write("\n");
        wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
        if (i < data.length - 1) wstream.write(',');
    }

    wstream.write('\n};')
    wstream.end();

    deleteAsync([source]);
}

const all = series(clean, buildfs_inline, buildfs_embeded, buildfs_yarrboard_logo_gz, buildfs_frothfet_logo_gz, buildfs_brineomatic_logo_gz, buildfs_logo_embedded);

export default all;
//export build;
