let brotliHost = 'http://udpx.fasani.de/tv/js/'
let brotliMime = 'mime.php?f=' 
let relativejs = 'js/'
! function(e, n) {
    "object" == typeof exports && "object" == typeof module ? module.exports = n() : "function" == typeof define && define.amd ? define([], n) : "object" == typeof exports ? exports.brotliwasm = n() : e.brotliwasm = n()
}(window, function() {
    return function(e) {
        function n(n) {
            for (var t, o, i = n[0], u = n[1], a = 0, f = []; a < i.length; a++) o = i[a], r[o] && f.push(r[o][0]), r[o] = 0;
            for (t in u) Object.prototype.hasOwnProperty.call(u, t) && (e[t] = u[t]);
            for (c && c(n); f.length;) f.shift()()
        }
        var t = {},
            r = {
                1: 0
            };
        var o = {};
        var i = {
            7: function() {
                return {
                    "./brotli_wasm_bindgen": {
                        __wbindgen_log2f: function(e) {
                            return t[1].exports.__wbindgen_log2f(e)
                        },
                        __wbindgen_log2: function(e) {
                            return t[1].exports.__wbindgen_log2(e)
                        },
                        __wbindgen_throw: function(e, n) {
                            return t[1].exports.__wbindgen_throw(e, n)
                        }
                    }
                }
            }
        };

        function u(n) {
            if (t[n]) return t[n].exports;
            var r = t[n] = {
                i: n,
                l: !1,
                exports: {}
            };
            return e[n].call(r.exports, r, r.exports, u), r.l = !0, r.exports
        }
        u.e = function(e) {
            var n = [],
                t = r[e];
            if (0 !== t)
                if (t) n.push(t[2]);
                else {
                    var a = new Promise(function(n, o) {
                        t = r[e] = [n, o]
                    });
                    n.push(t[2] = a);
                    var f, s = document.getElementsByTagName("head")[0],
                        c = document.createElement("script");
                    c.charset = "utf-8", c.timeout = 120, u.nc && c.setAttribute("nonce", u.nc), c.src = function(e) {
                        return u.p  + relativejs + e + ".brotli-wasm.js"
                    }(e), f = function(n) {
                        c.onerror = c.onload = null, clearTimeout(l);
                        var t = r[e];
                        if (0 !== t) {
                            if (t) {
                                var o = n && ("load" === n.type ? "missing" : n.type),
                                    i = n && n.target && n.target.src,
                                    u = new Error("Loading chunk " + e + " failed.\n(" + o + ": " + i + ")");
                                u.type = o, u.request = i, t[1](u)
                            }
                            r[e] = void 0
                        }
                    };
                    var l = setTimeout(function() {
                        f({
                            type: "timeout",
                            target: c
                        })
                    }, 12e4);
                    c.onerror = c.onload = f, s.appendChild(c)
                }
            return ({
                0: [7]
            }[e] || []).forEach(function(e) {
                var t = o[e];
                if (t) n.push(t);
                else {
                    var r, a = i[e](),
                        f = fetch(u.p + "" + {
                            7: brotliHost+brotliMime+"165db525e6e2c2f2aa80"
                        }[e] + ".module.wasm");
                    if (a instanceof Promise && "function" == typeof WebAssembly.compileStreaming) r = Promise.all([WebAssembly.compileStreaming(f), a]).then(function(e) {
                        return WebAssembly.instantiate(e[0], e[1])
                    });
                    else if ("function" == typeof WebAssembly.instantiateStreaming) r = WebAssembly.instantiateStreaming(f, a);
                    else {
                        r = f.then(function(e) {
                            return e.arrayBuffer()
                        }).then(function(e) {
                            return WebAssembly.instantiate(e, a)
                        })
                    }
                    n.push(o[e] = r.then(function(n) {
                        return u.w[e] = (n.instance || n).exports
                    }))
                }
            }), Promise.all(n)
        }, u.m = e, u.c = t, u.d = function(e, n, t) {
            u.o(e, n) || Object.defineProperty(e, n, {
                enumerable: !0,
                get: t
            })
        }, u.r = function(e) {
            "undefined" != typeof Symbol && Symbol.toStringTag && Object.defineProperty(e, Symbol.toStringTag, {
                value: "Module"
            }), Object.defineProperty(e, "__esModule", {
                value: !0
            })
        }, u.t = function(e, n) {
            if (1 & n && (e = u(e)), 8 & n) return e;
            if (4 & n && "object" == typeof e && e && e.__esModule) return e;
            var t = Object.create(null);
            if (u.r(t), Object.defineProperty(t, "default", {
                    enumerable: !0,
                    value: e
                }), 2 & n && "string" != typeof e)
                for (var r in e) u.d(t, r, function(n) {
                    return e[n]
                }.bind(null, r));
            return t
        }, u.n = function(e) {
            var n = e && e.__esModule ? function() {
                return e.default
            } : function() {
                return e
            };
            return u.d(n, "a", n), n
        }, u.o = function(e, n) {
            return Object.prototype.hasOwnProperty.call(e, n)
        }, u.p = "", u.oe = function(e) {
            throw console.error(e), e
        }, u.w = {};
        var a = window.webpackJsonpbrotliwasm = window.webpackJsonpbrotliwasm || [],
            f = a.push.bind(a);
        a.push = n, a = a.slice();
        for (var s = 0; s < a.length; s++) n(a[s]);
        var c = f;
        return u(u.s = 0)
    }([function(e, n, t) {
        "use strict";

        function r() {
            return t.e(0).then(t.bind(null, 1))
        }
        t.r(n), t.d(n, "load", function() {
            return r
        })
    }])
});
//# sourceMappingURL=brotli-wasm.js.map