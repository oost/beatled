const dns = require('dns');
dns.setDefaultResultOrder('ipv4first');

fetch(process.argv[2]).then(async (res) => {
  console.log(await res.text());
})