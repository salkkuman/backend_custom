# C Authentication Backend

A lightweight HTTP server written in C that provides admin authentication for frontend applications. Perfect for resource-constrained environments like Linode Nanode VPS.

## Features

- **Session-based Authentication** - Secure token-based login system
- **Admin Protection** - Protected endpoints requiring valid tokens
- **CORS Support** - Ready for frontend integration
- **Session Management** - Automatic session cleanup and timeout
- **Minimal Dependencies** - Only uses standard C libraries
- **Low Resource Usage** - Ideal for 1GB RAM servers

## Requirements

- **Operating System:** Linux (Ubuntu/Debian recommended)
- **Compiler:** GCC with C99 support
- **Libraries:** Standard C libraries + crypt library
- **RAM:** ~5-10MB memory usage
- **Storage:** <1MB compiled binary

## Quick Start

### 1. Clone or Download Files

```bash
git clone <your-repo-url>
cd c-auth-backend
```

### 2. Install Dependencies (Ubuntu/Debian)

```bash
make install-deps
# Or manually:
sudo apt-get update
sudo apt-get install build-essential
```

### 3. Compile and Run

```bash
# Compile
make

# Run the server
make run

# Or run manually
./auth_server
```

The server will start on `http://localhost:8080`

## API Endpoints

### Authentication

#### POST `/login`
Login with admin credentials and receive a session token.

**Request:**
```json
{
  "username": "admin",
  "password": "your_secure_password"
}
```

**Response (Success):**
```json
{
  "success": true,
  "token": "abc123...",
  "message": "Login successful"
}
```

**Response (Error):**
```json
{
  "success": false,
  "message": "Invalid credentials"
}
```

### Protected Routes

#### GET `/admin`
Access protected admin data (requires authentication).

**Headers:**
```
Authorization: Bearer <your-token>
```

**Response (Success):**
```json
{
  "success": true,
  "message": "Access granted",
  "data": "Protected admin data"
}
```

### Public Routes

#### GET `/`
API information and available endpoints.

**Response:**
```json
{
  "message": "C Backend API",
  "endpoints": ["/login", "/admin"]
}
```

## Configuration

### Default Credentials
**⚠️ IMPORTANT: Change these before production!**

- **Username:** `admin`
- **Password:** `your_secure_password`

Edit these in `auth_server.c`:
```c
const char* ADMIN_USERNAME = "admin";
const char* ADMIN_PASSWORD = "your_secure_password";
```

### Session Settings
- **Timeout:** 1 hour (3600 seconds)
- **Max Sessions:** 100 concurrent sessions
- **Port:** 8080 (configurable in code)

## Frontend Integration

Use the included `test.html` file to test the authentication flow, or integrate with your frontend:

### JavaScript Example

```javascript
// Login
const response = await fetch('http://localhost:8080/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ username: 'admin', password: 'your_secure_password' })
});
const data = await response.json();
const token = data.token;

// Access protected endpoint
const protectedResponse = await fetch('http://localhost:8080/admin', {
    headers: { 'Authorization': `Bearer ${token}` }
});
```

### React/Vue/Angular Integration

The server includes CORS headers, so it works seamlessly with modern frontend frameworks running on different ports.

## File Structure

```
project/
├── auth_server.c          # Main server code
├── Makefile              # Build configuration
├── test.html             # Frontend test page
└── README.md             # This file
```

## Makefile Commands

```bash
make                 # Compile the server
make run            # Compile and run
make clean          # Remove compiled files
make install-deps   # Install build dependencies
```

## Production Deployment

### 1. Security Hardening

```c
// Change default credentials
const char* ADMIN_USERNAME = "your_admin_username";
const char* ADMIN_PASSWORD = "your_strong_password_here";

// Consider adding password hashing
// Use environment variables for secrets
```

### 2. Systemd Service (Optional)

Create `/etc/systemd/system/auth-server.service`:

```ini
[Unit]
Description=C Authentication Server
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/path/to/your/server
ExecStart=/path/to/your/server/auth_server
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable auth-server
sudo systemctl start auth-server
```

### 3. Reverse Proxy with Nginx

```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    location /api/ {
        proxy_pass http://localhost:8080/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
    
    location / {
        # Serve your frontend
        root /var/www/html;
        try_files $uri $uri/ /index.html;
    }
}
```

## Performance & Resource Usage

### Benchmarks
- **Memory Usage:** ~5-10MB RAM
- **CPU Usage:** <1% on idle
- **Concurrent Connections:** Supports multiple clients
- **Session Storage:** In-memory (100 sessions max)

### Scaling Considerations
For higher traffic, consider:
- Database session storage
- Connection pooling
- Load balancing
- Redis for session management

## Troubleshooting

### Common Issues

**Compilation Errors:**
```bash
# Missing compiler
sudo apt-get install build-essential

# Missing crypt library
sudo apt-get install libc6-dev
```

**Port Already in Use:**
```bash
# Check what's using port 8080
sudo lsof -i :8080

# Kill the process or change port in code
```

**Permission Denied:**
```bash
# Make binary executable
chmod +x auth_server

# Or run with sudo if binding to port <1024
sudo ./auth_server
```

### Debugging

Enable debug output by adding prints:
```c
printf("Request received: %s\n", buffer);
printf("Session created: %s\n", token);
```

## Security Considerations

### ⚠️ Important Security Notes

1. **Change Default Credentials** - Never use default username/password in production
2. **Use HTTPS** - Deploy behind reverse proxy with SSL/TLS
3. **Password Hashing** - Consider bcrypt or similar for password storage
4. **Input Validation** - The current implementation has basic validation
5. **Rate Limiting** - Add rate limiting for login attempts
6. **Logging** - Implement proper logging for security monitoring

### Production Security Checklist

- [ ] Changed default admin credentials
- [ ] Deployed behind HTTPS reverse proxy
- [ ] Implemented proper password hashing
- [ ] Added rate limiting
- [ ] Set up monitoring and logging
- [ ] Configured firewall rules
- [ ] Regular security updates

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is open source. Feel free to modify and distribute as needed.

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review the code comments
3. Test with the included HTML file
4. Open an issue if problems persist

---

**Note:** This is a minimal authentication server suitable for learning and small projects. For production applications handling sensitive data, consider using established frameworks with comprehensive security features.
