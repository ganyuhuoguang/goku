const router = require('express').Router();
const task = require('./task');
router.use('/task', task);
module.exports = router;
