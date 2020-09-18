package com.app.gifplay

import android.graphics.Bitmap
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.util.Log
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.bumptech.glide.Glide
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File

class MainActivity : AppCompatActivity() {

    lateinit var bitmap: Bitmap;
    lateinit var gifHandler: GifHandler;

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        button.setOnClickListener { ndkLoadGif(it) }
    }

    val handler: Handler = object : Handler() {
        override fun handleMessage(msg: Message) {
            val nextFrame = gifHandler.updateFrame(bitmap);
            sendEmptyMessageDelayed(1, nextFrame.toLong())
            image.setImageBitmap(bitmap)
        }
    }

    fun ndkLoadGif(view: View) {
        val file = File(Environment.getExternalStorageDirectory(), "demo.gif")
        gifHandler = GifHandler(file.absolutePath)

        val width = gifHandler.getWidth();
        val height = gifHandler.getHeight();
        bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

        val nextFrame = gifHandler.updateFrame(bitmap);
        handler.sendEmptyMessageDelayed(1, nextFrame.toLong())
    }
}
