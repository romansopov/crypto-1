/// <reference types="node" />
/// <reference types="request" />
/// <reference types="async" />
/// <reference path="../native.ts" />
/// <reference path="../object.ts" />

namespace trusted.pki {

    const request = require("request");
    const fs = require("fs");
    const async = require("async");

    /**
     * Download file
     *
     * @param {string} url Url to remote file
     * @param {string} path Path for save in local system
     * @param {Function} done callback function
     */
    function download(url: string, path: string, done: (err: Error, url?: string, path?: string) => void): void {
        "use strict";

        const sendReq: any = request.get(url);

        sendReq.on("response", function(response) {
            switch (response.statusCode) {
                case 200:
                    const stream = fs.createWriteStream(path);

                    response.on("data", function(chunk) {
                        stream.write(chunk);
                    }).on("end", function() {
                        stream.on("close", function() {
                            done(null, url, path);
                        });
                        stream.end();
                    });

                    break;
                default:
                    done(new Error("Server responded with status code" + response.statusCode));
            }
        });

        sendReq.on("error", function(err) {
            fs.unlink(path);
            done(err.message);
        });
    }

    /**
     * Revocatiom provaider
     *
     * @export
     * @class Revocation
     * @extends {BaseObject<native.PKI.Revocation>}
     */
    export class Revocation extends BaseObject<native.PKI.Revocation> {
        /**
         * Creates an instance of Revocation.
         *
         *
         * @memberOf Revocation
         */
        constructor() {
            super();
            this.handle = new native.PKI.Revocation();
        }

        /**
         *  Search crl for certificate in local store
         *
         * @param {Certificate} cert
         * @param {PkiStore} store Local store
         * @returns {*}
         *
         * @memberOf Revocation
         */
        public getCrlLocal(cert: Certificate, store: pkistore.PkiStore): any {
            const res = this.handle.getCrlLocal(cert.handle, store.handle);
            if (res) {
                return Crl.wrap<native.PKI.CRL, Crl>(res);
            }
            return res;
        }

        /**
         * Return array of distribution points for certificate
         *
         * @param {Certificate} cert
         * @returns {Array<string>}
         *
         * @memberOf Revocation
         */
        public getCrlDistPoints(cert: Certificate): string[] {
            return this.handle.getCrlDistPoints(cert.handle);
        }

        /**
         * Check validate CRL time
         *
         * @param {Crl} crl
         * @returns {boolean}
         *
         * @memberOf Revocation
         */
        public checkCrlTime(crl: Crl): boolean {
            return this.handle.checkCrlTime(crl.handle);
        }

        /**
         * Download CRl
         *
         * @param {Array<string>} distPoints Distribution points
         * @param {string} pathForSave File path
         * @param {Function} done callback
         *
         * @memberOf Revocation
         */
        public downloadCRL(distPoints: string[], pathForSave: string, done: (err: Error, crl: Crl) => void): void {
            const crl = new Crl();
            let returnPath;

            try {
                async.forEachOf(distPoints, function(value, key, callback) {
                    download(value, pathForSave + key, function(err, url, goodPath) {
                        if (err) {
                            return callback(err);
                        } else {
                            returnPath = goodPath;
                            callback();
                        }
                    });
                }, function(err) {
                    if (err) {
                        done(err.message, null);
                    } else {
                        crl.load(returnPath);
                        done(null, crl);
                    }
                });
            } catch (e) {
                done(e, null);
            }
        }
    }
}
