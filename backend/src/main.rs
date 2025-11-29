use libp2p::Multiaddr;
use std::error::Error;

mod crypto;
mod messaging;
mod network;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    env_logger::init();
    
    println!("SecureComm Backend Starting...");
    println!("Privacy-first communication platform");
    
    // Initialize cryptographic identity
    let identity = crypto::Identity::generate();
    println!("Node ID: {}", identity.peer_id());
    
    // Initialize network
    let mut network = network::SecureCommNetwork::new(identity).await?;
    
    // Start listening on localhost
    network.start_listening("/ip4/0.0.0.0/tcp/0").await?;
    
    println!("SecureComm backend initialized successfully");
    println!("Use Ctrl+C to shutdown");
    
    // Start network event loop in background
    tokio::spawn(async move {
        network.run_event_loop().await;
    });
    
    // Keep the node running
    tokio::signal::ctrl_c().await?;
    println!("Shutting down...");
    
    Ok(())
}
