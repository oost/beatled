const { createProxyMiddleware } = require("http-proxy-middleware");
const dns = require("dns");
dns.setDefaultResultOrder("ipv4first");

module.exports = function (app) {
  app.use(
    "/api",
    createProxyMiddleware({
      target: "http://localhost:8080/",
      changeOrigin: true,
    })
  );
};
