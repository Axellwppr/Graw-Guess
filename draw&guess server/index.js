import { createServer } from "http";
import { Server } from "socket.io";

const httpServer = createServer();
const io = new Server(httpServer, {
    // options
});

let primary = null, nowProblem = null, counter = null, t = 45, cls = 0;
let problem = ["云", "太阳", "书", "手机", "台灯", "猫", "雪花"];

var newProblem = () => {
    var rand = Math.floor(Math.random() * problem.length);
    return problem[rand];
}

io.on("connection", (socket) => {
    console.log(`clients list updated, ${++cls} online now`);
    socket.on("disconnect", () => {
        console.log(`clients list updated, ${--cls} online now`);
    })

    socket.on("statusUpdate", msg => {
        if (primary && socket.id == primary) {
            console.log("forward", msg);
            socket.broadcast.emit("remoteStatusChanged", msg);
        } else {
            console.log("reject")
        }
    })
    socket.on("requestPromote", msg => {
        clearInterval(counter);
        primary = socket.id;
        socket.broadcast.emit("promotionChanged");
        socket.emit("promotionChangedByself");
        console.log("client request promotion");
    })
    socket.on("requestProblem", () => {
        if (primary && socket.id == primary) {
            nowProblem = newProblem();
            socket.emit("problemChangedByself", nowProblem);
            socket.broadcast.emit("problemChanged");
            clearInterval(counter);
            t = 45;
            io.emit("countdownChanged", t + '');
            counter = setInterval(() => {
                t--;
                io.emit("countdownChanged", t + '');
                if (t == 0) {
                    io.emit("failed");
                    nowProblem = null;
                    clearInterval(counter);
                }
            }, 1000);
        } else {
            console.log("reject")
        }
    })
    socket.on("submit", (answer) => {
        if (answer == nowProblem && nowProblem != null) {
            io.emit("succeeded");
            clearInterval(counter);
            nowProblem = null;
        } else {
            socket.emit("incorrect");
        }
    })

});

httpServer.listen(3000, () => {
    console.log('listening on *:3000');
});