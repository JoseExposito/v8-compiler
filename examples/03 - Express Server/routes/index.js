const express  = require('express')
const router   = express.Router()
const template = require('../views/index.pug')

/* GET home page. */
router.get('/', function(req, res, next) {
    let html = template({ title: 'Express + Pug + Webpack' })
    res.send(html)
})

module.exports = router
