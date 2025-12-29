/*

Yarrboard Framework HTML Builder

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>;
Copyright (C) 2025 by Zach Hoeken <hoeken@gmail.com>;

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

import gulp from 'gulp';
const { series, src, dest } = gulp;
import htmlmin from 'gulp-htmlmin';
import cleancss from 'gulp-clean-css';
// import uglify from 'gulp-uglify-es';
import gzip from 'gulp-gzip';
import { deleteAsync } from 'del';
import inline from 'gulp-inline';
import inlineImages from 'gulp-css-base64';
import favicon from 'gulp-base64-favicon';
import { readFileSync, createWriteStream } from 'fs';
import { createHash } from 'crypto';
import { join } from 'path';

// ============================================================================
// Configuration
// ============================================================================

// Get the YarrboardFramework path from environment variable
// This is set by the gulp.py script when run via PlatformIO
const FRAMEWORK_PATH = process.env.YARRBOARD_FRAMEWORK_PATH;

if (!FRAMEWORK_PATH) {
    console.error('ERROR: YARRBOARD_FRAMEWORK_PATH environment variable not set!');
    console.error('This should be set by scripts/gulp.py when running via PlatformIO.');
    process.exit(1);
}

console.log(`Using YarrboardFramework from: ${FRAMEWORK_PATH}`);

const PATHS = {
    frameworkHtml: join(FRAMEWORK_PATH, 'html'),  // Framework HTML/CSS/JS
    projectHtml: 'html',                           // Project-specific assets (logos)
    dist: 'dist',
    src: 'src/gulp'
};

const LOGOS = [
    'logo-sendit.png',
    'logo-brineomatic.png',
    'logo-frothfet.png'
];

const HTML_MIN_OPTIONS = {
    removeComments: true,
    minifyCSS: true,
    minifyJS: true
};

const INLINE_OPTIONS = {
    base: PATHS.frameworkHtml,
    css: [cleancss, inlineImages],
    ignore: [
        'logo.png',        // ignore the apple-touch-icon line
        'site.webmanifest' // ignore the manifest line
    ]
};

// ============================================================================
// Utility Functions
// ============================================================================

async function writeHeaderFile(source, destination, name) {
    return new Promise((resolve, reject) => {
        try {
            const wstream = createWriteStream(destination);
            const data = readFileSync(source);

            const hashSum = createHash('sha256');
            hashSum.update(data);
            const hex = hashSum.digest('hex');

            wstream.write(`#define ${name}_len ${data.length}\n`);
            wstream.write(`const char ${name}_sha[] = "${hex}";\n`);
            wstream.write(`const uint8_t ${name}[] = {`);

            for (let i = 0; i < data.length; i++) {
                if (i % 1000 === 0) wstream.write("\n");
                wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
                if (i < data.length - 1) wstream.write(',');
            }

            wstream.write('\n};');
            wstream.end();

            wstream.on('finish', async () => {
                await deleteAsync([source], { force: true });
                resolve();
            });

            wstream.on('error', reject);
        } catch (err) {
            reject(err);
        }
    });
}

// ============================================================================
// Gulp Tasks
// ============================================================================

async function clean() {
    return deleteAsync([join(PATHS.dist, '*')], { force: true });
}

function buildInlineHtml() {
    return src(join(PATHS.frameworkHtml, '*.html'))
        .pipe(favicon({ src: PATHS.frameworkHtml }))
        .pipe(inline(INLINE_OPTIONS))
        .pipe(htmlmin(HTML_MIN_OPTIONS))
        .pipe(gzip())
        .pipe(dest(PATHS.dist));
}

async function embedHtml() {
    const source = join(PATHS.dist, 'index.html.gz');
    const destination = join(PATHS.src, 'index.html.gz.h');
    await writeHeaderFile(source, destination, 'index_html_gz');
}

function compressLogo(filename) {
    return src(join(PATHS.projectHtml, filename))
        .pipe(gzip())
        .pipe(dest(PATHS.dist));
}

async function embedLogo(filename) {
    const source = join(PATHS.dist, `${filename}.gz`);
    const destination = join(PATHS.src, `${filename}.gz.h`);
    const safeName = filename.replace(/[^a-z0-9]/gi, '_') + '_gz';

    await writeHeaderFile(source, destination, safeName);
}

function createLogoTask(filename) {
    const compress = () => compressLogo(filename);
    const embed = () => embedLogo(filename);

    return series(compress, embed);
}

// ============================================================================
// Task Composition
// ============================================================================

const logoTasks = LOGOS.map(logo => createLogoTask(logo));
const buildAll = series(
    clean,
    buildInlineHtml,
    embedHtml,
    ...logoTasks
);

// ============================================================================
// Exports
// ============================================================================

export {
    clean,
    buildInlineHtml,
    embedHtml,
    buildAll as default
};
